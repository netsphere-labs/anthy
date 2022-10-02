#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#ifndef _WIN32
  #include <unistd.h>
  #include <sys/mman.h>  // mmap()
#else
  #define STRICT 1
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include <anthy/diclib.h>
#include <anthy/alloc.h>
#include <anthy/conf.h>
#include <anthy/logger.h>
#include "diclib_inner.h"

/**
   複数セクションがリンクされた辞書
 */
struct file_dic
{
  void *ptr;
  size_t size;
#ifdef _WIN32
  HANDLE hMap;
#endif
};

static struct file_dic fdic;


/**
 * @return the pointer to contents. If failed, NULL.
 */
void*
anthy_file_dic_get_section(const char* section_name)
{
  assert(section_name);

  int i;
  char* head = (char *)fdic.ptr;
  int* p = (int*)head;
  int entry_num = anthy_dic_ntohl(*p++);

  for (i = 0; i < entry_num; ++i) {
    int hash_offset = anthy_dic_ntohl(*p++);
    int key_len =  anthy_dic_ntohl(*p++);
    int contents_offset = anthy_dic_ntohl(*p++);
    if (hash_offset > fdic.size || contents_offset > fdic.size) {
      abort(); // invalid data.
    }
    if (strncmp(section_name, head + hash_offset, key_len) == 0) {
      return (void*)(head + contents_offset);
    }
  }
  return NULL;
}


/**
 * map a file to memory. No need to share inter-process.
 * @param fn file name.
 * @return 0 if succeeded, -1 if failed.
 */
static int
anthy_mmap (const char *fn)
{
#ifndef _WIN32
  int fd;
  struct stat st;
#else
  HANDLE fd;
  HANDLE hMap;
  LARGE_INTEGER st;
#endif
  void *ptr;

#ifndef _WIN32
  fd = open (fn, O_RDONLY);
  if (fd == -1) {
    anthy_log(0, "Failed to open (%s).\n", fn);
    return -1;
  }

  if (fstat(fd, &st) < 0) {
    anthy_log(0, "Failed to stat() (%s).\n", fn);
    close(fd);
    return -1;
  }
  if (st.st_size == 0) {
    anthy_log(0, "Failed to mmap 0size file (%s).\n", fn);
    close(fd);
    return -1;
  }

  ptr = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (ptr == MAP_FAILED) {
    anthy_log(0, "Failed to mmap() (%s).\n", fn);
    return -1;
  }

  fdic.size = st.st_size;
#else
  fd = CreateFileA(fn, GENERIC_READ,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, // lpSecurityAttributes
                   OPEN_EXISTING,
                   0, // dwFlagsAndAttributes. OPEN_EXISTING の場合は無視される
                   NULL);
  if (fd == INVALID_HANDLE_VALUE) {
    anthy_log(0, "Failed to open (%s).\n", fn);
    return -1;
  }

  if (!GetFileSizeEx(fd, &st)) {
    anthy_log(0, "Failed to stat (%s).\n", fn);
    CloseHandle(fd);
    return -1;
  }
  if (!st.QuadPart) {
    anthy_log(0, "Failed to mmap 0 size file (%s).\n", fn);
    CloseHandle(fd);
    return -1;
  }

  hMap = CreateFileMappingA(fd,
                        NULL, // lpFileMappingAttributes
                        PAGE_READONLY,
                        0, 0, // 既存ファイルの大きさ
                        NULL); // lpName. "メモリ"は共有しない.
  if (!hMap || hMap == INVALID_HANDLE_VALUE) {
    anthy_log(0, "Failed to CreateFileMapping() (%s).\n", fn);
    CloseHandle(fd);
    return -1;
  }
  ptr = MapViewOfFile(hMap,
                      FILE_MAP_READ,
                      0, 0, // offset
                      0); // dwNumberOfBytesToMap. To the end of the mapping.
  CloseHandle(fd);
  if (!ptr) {
    anthy_log(0, "Failed to mmap (%s).\n", fn);
    CloseHandle(hMap);
    return -1;
  }

  fdic.size = st.QuadPart;
  fdic.hMap = hMap;
#endif
  fdic.ptr = ptr;

  return 0;
}

int
anthy_init_file_dic(void)
{
  const char *fn;
  fn = anthy_conf_get_str("DIC_FILE");
  printf("DIC_FILE = %s\n", fn);
  if (!fn) {
    anthy_log(0, "dictionary is not specified.\n");
    return -1;
  }

  /* 辞書をメモリ上にmapする */
  if (anthy_mmap (fn) < 0) {
    anthy_log(0, "failed to init file dic.\n");
    return -1;
  }

  return 0;
}

void
anthy_quit_file_dic(void)
{
#ifndef _WIN32
  munmap (fdic.ptr, fdic.size);
#else
  UnmapViewOfFile(fdic.ptr);
  CloseHandle(fdic.hMap);
  fdic.hMap = INVALID_HANDLE_VALUE;
#endif
}

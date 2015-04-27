#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

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
};

static struct file_dic fdic;

void*
anthy_file_dic_get_section(const char* section_name)
{
  int i;
  char* head = (char *)fdic.ptr;
  int* p = (int*)head;
  int entry_num = anthy_dic_ntohl(*p++);
  
  for (i = 0; i < entry_num; ++i) {
    int hash_offset = anthy_dic_ntohl(*p++);
    int key_len =  anthy_dic_ntohl(*p++);
    int contents_offset = anthy_dic_ntohl(*p++);
    if (strncmp(section_name, head + hash_offset, key_len) == 0) {
      return (void*)(head + contents_offset);
    }
  }
  return NULL;
}

static int
anthy_mmap (const char *fn)
{
  int fd;
  void *ptr;
  struct stat st;

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

  fdic.ptr = ptr;
  fdic.size = st.st_size;
  return 0;
}

int
anthy_init_file_dic(void)
{
  const char *fn;
  fn = anthy_conf_get_str("DIC_FILE");
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
  munmap (fdic.ptr, fdic.size);
}


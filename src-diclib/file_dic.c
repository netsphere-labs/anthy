#include <stdio.h>
#include <string.h>

#include <anthy/diclib.h>
#include <anthy/filemap.h>
#include <anthy/alloc.h>
#include <anthy/conf.h>
#include <anthy/logger.h>
#include "diclib_inner.h"

/**
   複数セクションがリンクされた辞書
 */
struct file_dic
{
  struct filemapping *mapping;
};

static struct file_dic fdic;

void*
anthy_file_dic_get_section(const char* section_name)
{
  int i;
  char* head = anthy_mmap_address(fdic.mapping);
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
  fdic.mapping = anthy_mmap(fn, 0);
  if (!fdic.mapping) {
    anthy_log(0, "failed to init file dic.\n");
    return -1;
  }

  return 0;
}

void
anthy_quit_file_dic(void)
{
  anthy_munmap(fdic.mapping);  
}


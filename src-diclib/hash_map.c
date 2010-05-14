/*
 * Anthy§Œ»∆Õ—§Œhash
 * Copyright (C) 2004 TABATA Yusuke
 */
#include <conf.h>
#include <hash_map.h>
#include "dic_main.h"
#include "file_dic.h"

static char *shared_hash_map;

void
anthy_init_hashmap(struct file_dic *fdic)
{
  shared_hash_map = anthy_file_dic_get_hashmap_ptr(fdic);
}

void anthy_quit_hashmap(void)
{
  shared_hash_map = 0;
}

int
anthy_word_relation_hash(int from, int to)
{
  int hash = from + to;
  hash %= VERSATILE_HASH_SIZE;
  return hash;
}

int
anthy_check_word_relation(int from, int to)
{
  int hash = anthy_word_relation_hash(from, to);
  if (!shared_hash_map) {
    return 0;
  }
  return shared_hash_map[hash];
}

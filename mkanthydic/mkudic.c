/*
 * 用例辞書を作る
 *
 * Copyright (C) 2003-2004 TABATA Yusuke
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <file_dic.h>
#include "mkdic.h"
#include "hash_map.h"

#define LINE_LEN 256


struct use_case {
  int id[2];
  struct use_case *next;
};

struct uc_dict {
  struct use_case uc_head;
  int nr_ucs;
  struct yomi_entry *entry_list;
};

/*
 * 単語のidを見付ける
 */
static int
find_word_id(struct uc_dict *dict, char *yomi, char *word, char *wt)
{
  struct yomi_entry *ye;
  xstr *xs = anthy_cstr_to_xstr(yomi, 0);
  for (ye = dict->entry_list; ye; ye = ye->next) {
    if (!anthy_xstrcmp(ye->index_xstr, xs)) {
      int i;
      for (i = 0; i < ye->nr_entries; i++) {
	struct word_entry *we = &ye->entries[i];
	if (!strcmp(word, we->word) &&
	    !strcmp(wt, we->wt)) {
	  anthy_free_xstr(xs);
	  return we->offset;
	}
      }
    }
  }
  anthy_free_xstr(xs);
  printf("Invalid word in ucdict %s %s %s.\n", yomi, word, wt);
  return -1;
}

/* 用例定義の行から単語のidを求める
 * 見つからなければ -1
 */
static int
get_id_from_word_line(struct uc_dict *dict, char *buf)
{
  char yomi[LINE_LEN];
  char okuri[LINE_LEN];
  char wt[LINE_LEN];
  char kanji[LINE_LEN];
  int res;

  res = sscanf(buf, "%s %s %s %s", yomi, okuri, wt, kanji);
  if (res != 4) {
    return -1;
  }
  return find_word_id(dict, yomi, kanji, wt);
}

static void
commit_uc(struct uc_dict *dict, int x, int y)
{
  struct use_case *uc;
  if (x < 0 || y < 0) {
    return ;
  }
  uc = malloc(sizeof(struct use_case));
  uc->id[0] = x;
  uc->id[1] = y;
  /**/
  uc->next = dict->uc_head.next;
  dict->uc_head.next = uc;
  dict->nr_ucs ++;
}

/* 用例ファイルを読み込む */
struct uc_dict *
read_uc_file(const char *fn, struct yomi_entry *ye)
{
  char buf[LINE_LEN];
  FILE *uc_file;
  int off, base = 0, cur;
  struct uc_dict *dict;

  dict = malloc(sizeof(struct uc_dict));
  dict->entry_list = ye;
  dict->uc_head.next = NULL;
  dict->nr_ucs = 0;


  uc_file = fopen(fn, "r");
  if (!uc_file) {
    return dict;
  }

  /* off=0      : 最初の単語
   * off=1,2..n : それと関係ある単語
   */
  off = 0;
  while (fgets(buf, LINE_LEN, uc_file)) {
    /**/
    if (buf[0] == '#') {
      continue;
    }
    if (buf[0] == '-') {
      off = 0;
      continue;
    }
    cur = get_id_from_word_line(dict, buf);
    if (off == 0) {
      base = cur;
    } else {
      commit_uc(dict, cur, base);
    }
    off ++;
  }
  return dict;
}

void
make_ucdic(FILE *uc_out, struct uc_dict *dict)
{
  struct use_case *uc;
  /**/
  write_nl(uc_out, 0x75646963);/*MAGIC udic*/
  write_nl(uc_out, 0);/*Version*/
  write_nl(uc_out, 16);/*Header Size*/
  write_nl(uc_out, dict->nr_ucs);
  for (uc = dict->uc_head.next; uc; uc = uc->next) {
    write_nl(uc_out, uc->id[0]);
    write_nl(uc_out, uc->id[1]);
  }
  printf("udic: %d use examples.\n", dict->nr_ucs);
}

void
fill_uc_to_hash(struct versatile_hash *vh, struct uc_dict *dict)
{
  struct use_case *uc;
  /* 各用例を書き込む */
  for (uc = dict->uc_head.next; uc; uc = uc->next) {
    int hash = anthy_word_relation_hash(uc->id[0], uc->id[1]);
    vh->buf[hash] = 1;
  }
}

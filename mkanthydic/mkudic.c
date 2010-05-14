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

/* 用例 */
struct use_case {
  int id[2];
  struct use_case *next;
};

/* 用例辞書 */
struct uc_dict {
  /* 用例リスト */
  struct use_case uc_head;
  int nr_ucs;
  /* ソートするための配列 */
  struct use_case **uc_array;
  /* 単語の辞書 */
  struct yomi_entry_list *entry_list;
};

/*
 * 単語のidを見付ける
 */
static int
find_word_id(struct uc_dict *dict, char *yomi, char *word, char *wt)
{
  struct yomi_entry *ye;
  int i;
  xstr *xs = anthy_cstr_to_xstr(yomi, 0);
  ye = find_yomi_entry(dict->entry_list, xs, 0);
  anthy_free_xstr(xs);
  if (!ye) {
    return -1;
  }
  for (i = 0; i < ye->nr_entries; i++) {
    struct word_entry *we = &ye->entries[i];
    if (!strcmp(word, we->word) &&
	!strcmp(wt, we->wt)) {
      return we->offset;
    }
  }
  return -1;
}

/* 用例定義の行から単語のidを求める
 * 見つからなければ -1
 */
static int
get_id_from_word_line(struct uc_dict *dict, char *buf, int ln)
{
  char yomi[LINE_LEN];
  char okuri[LINE_LEN];
  char wt[LINE_LEN];
  char kanji[LINE_LEN];
  int res, id;

  res = sscanf(buf, "%s %s %s %s", yomi, okuri, wt, kanji);
  if (res != 4) {
    fprintf(stderr, "Invalid line(%d):%s\n", ln, buf);
    return -1;
  }
  id = find_word_id(dict, yomi, kanji, wt);
  if (id == -1) {
    fprintf(stderr, "Invalid word in ucdict (%d):%s %s %s.\n",
	    ln ,yomi, kanji, wt);
  }
  return id;
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

/* 用例データベースを作る */
struct uc_dict *
create_uc_dict(struct yomi_entry_list *yl)
{
  struct uc_dict *dict = malloc(sizeof(struct uc_dict));

  dict->entry_list = yl;
  dict->uc_head.next = NULL;
  dict->nr_ucs = 0;

  return dict;
}

/* 用例ファイルを読み込む */
void
read_uc_file(struct uc_dict *dict, const char *fn)
{
  char buf[LINE_LEN];
  FILE *uc_file;
  int off, base = 0, cur;
  int line_number = 0;

  uc_file = fopen(fn, "r");
  if (!uc_file) {
    return ;
  }

  /* off=0      : 最初の単語
   * off=1,2..n : それと関係ある単語
   */
  off = 0;
  while (fgets(buf, LINE_LEN, uc_file)) {
    /**/
    line_number ++;
    /**/
    if (buf[0] == '#') {
      continue;
    }
    if (buf[0] == '-') {
      off = 0;
      continue;
    }
    cur = get_id_from_word_line(dict, buf, line_number);
    if (off == 0) {
      base = cur;
    } else {
      commit_uc(dict, cur, base);
    }
    off ++;
  }
}

static int
compare_uc(const void *p1, const void *p2)
{
  const struct use_case *const *u1 = p1;
  const struct use_case *const *u2 = p2;
  int d;
  d = (*u1)->id[0] - (*u2)->id[0];
  if (d) {
    return d;
  }
  return (*u1)->id[1] - (*u2)->id[1];
}

static void
sort_udict(struct uc_dict *dict)
{
  int i;
  struct use_case *uc;
  /* 配列に追加 */
  dict->uc_array = malloc(sizeof(struct use_case *) *
			  dict->nr_ucs);
  for (i = 0, uc = dict->uc_head.next; uc; uc = uc->next) {
    dict->uc_array[i] = uc;
    i++;
  }
  /**/
  qsort((void *)dict->uc_array, dict->nr_ucs,
	sizeof(struct use_case *), compare_uc);
}

/* 用例辞書をファイルに書き出す */
void
make_ucdict(FILE *uc_out, struct uc_dict *dict)
{
  int i;
  struct use_case *uc;
  /**/
  write_nl(uc_out, 0x75646963);/*MAGIC udic*/
  write_nl(uc_out, 0);/*Version*/
  write_nl(uc_out, 16);/*Header Size*/
  if (!dict) {
    write_nl(uc_out, 0);
    printf("udic: no use examples.\n");
    return ;
  }
  /* sortする */
  sort_udict(dict);
  /**/
  write_nl(uc_out, dict->nr_ucs);
  for (i = 0; i < dict->nr_ucs; i++) {
    uc = dict->uc_array[i];
    write_nl(uc_out, uc->id[0]);
    write_nl(uc_out, uc->id[1]);
  }
  printf("udic: %d use examples.\n", dict->nr_ucs);
}

/* 用例辞書の内容をhashに書き込む */
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

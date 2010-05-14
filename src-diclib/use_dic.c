/*
 * 用例辞書を扱う 
 * Copyright (C) 2003 TABATA Yusuke
 */
#include <string.h>
#include <stdlib.h>

#include <dic.h>
#include <xstr.h>
#include <alloc.h>
#include <record.h>
#include <hash_map.h>
#include "dic_main.h"
#include "dic_ent.h"
#include "file_dic.h"

#define MAX_RELATION_COUNT 50

static allocator relation_ator;
extern struct mem_dic *anthy_current_personal_dic_cache;

/** 単語間の関係 */
struct relation {
  int from;
  int target;
  int used;
  struct relation *next;
};

static struct {
  struct relation *head;
} relation_list;

int anthy_file_dic_check_word_relation(struct file_dic *fdic,
				       int from, int to)
{
  struct relation *rel;
  int i;
  int res;
  (void)fdic;
  /* 個人辞書 */
  for (rel = relation_list.head; rel; rel = rel->next) {
    if (rel->from == from) {
      rel->used = 1;
      return 1;
    }
  }
  /* 共有辞書 */
  res = anthy_check_word_relation(from, to);
  if (!res) {
    return 0;
  }
  for (i = 0; i < fdic->nr_ucs; i++) {
    if (anthy_dic_ntohl(fdic->ucs[i * 2]) == from) {
      return 1;
    }
  }
  return 0;
}

static int
check_duplicated_relation(int from, int target)
{
  struct relation *rel;
  for (rel = relation_list.head; rel; rel = rel->next) {
    if (rel->from == from && rel->target == target) {
      return 1;
    }
  }
  return 0;
}

/** 単語の関係を登録する */
void
anthy_dic_register_relation(int from, int target)
{
  struct relation *rel;
  if (check_duplicated_relation(from, target)) {
    return ;
  }
  rel = anthy_smalloc(relation_ator);
  rel->from = from;
  rel->target = target;
  rel->used = 0;
  rel->next = relation_list.head;
  relation_list.head = rel;
}

static xstr *
word_id_to_xstr(int id)
{
  /*idの外部表現は「単語*品詞*読み」
   **/
  struct dic_ent *de =
    anthy_mem_dic_word_id_to_dic_ent(anthy_current_personal_dic_cache, id);
  struct seq_ent *se;
  xstr *res, *wt;
  if (!de) {
    return 0;
  }
  wt = anthy_cstr_to_xstr(de->wt_name, 0);
  se = de->se;
  /* de->str * de->type * se->str */
  res = anthy_xstr_dup(&de->str);
  res = anthy_xstrappend(res, '*');
  res = anthy_xstrcat(res, wt);
  res = anthy_xstrappend(res, '*');
  res = anthy_xstrcat(res, &se->str);
  anthy_free_xstr(wt);
  return res;
}

static int
find_aster(xstr *xs, int start)
{
  int i;
  for (i = start; i < xs->len; i++) {
    if (xs->str[i] == '*') {
      return i;
    }
  }
  return -1;
}

static int
do_xstr_to_word_id(xstr *word, char *wt_name, xstr *yomi)
{
  struct seq_ent *se;
  int i;
  se = anthy_get_seq_ent_from_xstr(yomi);
  if (!se) {
    return 0;
  }
  for (i = 0; i < se->nr_dic_ents; i++) {
    struct dic_ent *de = se->dic_ents[i];
    if (!strcmp(wt_name, de->wt_name) &&
	!anthy_xstrcmp(word, &de->str)) {
      return de->id;
    }
  }
  return 0;
}

static int
xstr_to_word_id(xstr *xs)
{
  xstr word_xs, wt_xs, yomi_xs;
  char *wt_name;
  int idx;
  int word_id;
  /* 「まず単語*品詞*読み」から切り出す */
  /* 単語を切り出す */
  word_xs.str = xs->str;
  idx = find_aster(xs, 0);
  if (idx == -1) {
    return 0;
  }
  word_xs.len = idx;
  /* 品詞名を切り出す */
  wt_xs.str = &xs->str[idx + 1];
  idx = find_aster(xs, idx + 1);
  if (idx == -1) {
    return 0;
  }
  wt_xs.len = idx - word_xs.len - 1;
  /* 読みを切り出す */
  yomi_xs.str = &xs->str[idx + 1];
  yomi_xs.len = xs->len - idx - 1;
  /**/
  wt_name = anthy_xstr_to_cstr(&wt_xs, 0);
  if (wt_name) {
    word_id = do_xstr_to_word_id(&word_xs, wt_name, &yomi_xs);
    free(wt_name);
    return word_id;
  }
  return 0;
}

static void
save_to_record(void)
{
  struct relation *rel;
  /* 今のデータを消す */
  if (anthy_select_section("WORD_RELATION", 1)) {
    return;
  }
  anthy_release_section();
  /**/
  if (anthy_select_section("WORD_RELATION", 1)) {
    return;
  }
  for (rel = relation_list.head; rel; rel = rel->next) {
    /**/
    xstr *from_xs, *target_xs;
    from_xs = word_id_to_xstr(rel->from);
    if (!from_xs) {
      continue;
    }
    target_xs = word_id_to_xstr(rel->target);
    if (!target_xs) {
      anthy_free_xstr(from_xs);
      continue;
    }
    if (!anthy_select_column(from_xs, 1)) {
      int nr = anthy_get_nr_values();
      anthy_set_nth_xstr(nr, target_xs);
    }
    anthy_free_xstr(from_xs);
    anthy_free_xstr(target_xs);
  }
}

/** 確定操作が行われたので、セーブする */
void
anthy_dic_commit_relation(void)
{
  int count = 0;
  struct relation *rel, *cur;
  for (rel = relation_list.head; rel->next;) {
    count ++;
    if (count < MAX_RELATION_COUNT) {
      /* 最初のMAX_RELATION_COUNTまではそのままにする */
      rel = rel->next;
      continue;
    }
    cur = rel->next;
    rel->next = cur->next;
    if (cur->used) {
      /* 先頭に持っていく */
      cur->used = 0;
      cur->next = relation_list.head;
      relation_list.head = cur;
    } else {
      /* 消す */
      anthy_sfree(relation_ator, cur);
    }
  }
  save_to_record();
}

static void
free_allocator(void)
{
  anthy_free_allocator(relation_ator);
  relation_ator = 0;
}

static void
init_list(void)
{
  relation_list.head = 0;
  if (relation_ator) {
    free_allocator();
  }
  relation_ator = anthy_create_allocator(sizeof(struct relation), 0);
}

void
anthy_dic_reload_use_dic(void)
{
  init_list();
  if (anthy_select_section("WORD_RELATION", 0)) {
    return;
  }
  if (anthy_select_first_column()) {
    return ;
  }
  do {
    int nr = anthy_get_nr_values();
    xstr *to_xs, *from_xs = anthy_get_index_xstr();
    int to_id, from_id;
    int i;
    from_id = xstr_to_word_id(from_xs);
    for (i = 0; i < nr; i++) {
      to_xs = anthy_get_nth_xstr(i);
      to_id = xstr_to_word_id(to_xs);
      anthy_dic_register_relation(from_id, to_id);
    }
  } while (!anthy_select_next_column());
}

void
anthy_init_use_dic(void)
{
  init_list();
}

void
anthy_quit_use_dic(void)
{
  free_allocator();
}

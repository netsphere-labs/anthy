/*
 * 用例辞書を作る
 *
 * Copyright (C) 2003-2005 TABATA Yusuke
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <anthy/matrix.h>
#include "mkdic.h"

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
};

/* 用例定義の行から単語のidを求める
 */
static int
get_id_from_word_line(char *buf)
{
  char yomi[LINE_LEN];
  char okuri[LINE_LEN];
  char wt[LINE_LEN];
  char kanji[LINE_LEN];
  int res, id;
  xstr *xs;

  res = sscanf(buf, "%s %s %s %s", yomi, okuri, wt, kanji);
  if (res != 4) {
    return -1;
  }
  xs = anthy_cstr_to_xstr(kanji, 0);
  id = anthy_xstr_hash(xs);
  anthy_free_xstr(xs);
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
create_uc_dict(void)
{
  struct uc_dict *dict = malloc(sizeof(struct uc_dict));

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
      /* コメント */
      continue;
    }
    if (buf[0] == '-') {
      /* 区切り記号 */
      off = 0;
      continue;
    }
    cur = get_id_from_word_line(buf);
    if (cur == -1) {
      fprintf(stderr, "Invalid line(%d):%s\n", line_number, buf);
    }
    /**/
    if (off == 0) {
      /* 一つめの項目 */
      base = cur;
    } else {
      /* 二つめ以降の項目 */
      commit_uc(dict, cur, base);
    }
    off ++;
  }
}

/* 用例辞書をファイルに書き出す */
void
make_ucdict(FILE *uc_out, struct uc_dict *dict)
{
  struct use_case *uc;
  struct sparse_matrix *sm;
  struct matrix_image *mi;
  int i;
  /* 疎行列に詰め込む */
  sm = anthy_sparse_matrix_new();
  if (dict) {
    for (uc = dict->uc_head.next; uc; uc = uc->next) {
      anthy_sparse_matrix_set(sm, uc->id[0], uc->id[1], 1, NULL);
    }
  }
  anthy_sparse_matrix_make_matrix(sm);
  /* 疎行列のイメージを作成してファイルに書き出す */
  mi = anthy_matrix_image_new(sm);
  for (i = 0; i < mi->size; i++) {
    write_nl(uc_out, mi->image[i]);
  }
  if (dict) {
    printf("udic: %d use examples.\n", dict->nr_ucs);
  } else {
    printf("udic: no use examples.\n");
  }

}

#ifndef _mkdic_h_included_
#define _mkdic_h_included_

#include <xstr.h>

/** 単語 */
struct word_entry {
  /** 品詞名 */
  char *wt;
  /** 頻度 */
  int freq;
  /** 単語 */
  char *word;
  /** オフセット */
  int offset;
};

/** ある読み */
struct yomi_entry {
  /* その読み*/
  xstr *index_xstr;
  /* 辞書ファイル中のページ中のオフセット */
  int offset;
  /* 各エントリ */
  int nr_entries;
  struct word_entry *entries;
  struct yomi_entry *next;
  struct yomi_entry *hash_next;
};

#define YOMI_HASH 1024

/* 辞書全体 */
struct yomi_entry_list {
  struct yomi_entry *head;
  int nr_entries;
  struct yomi_entry *hash[YOMI_HASH];
  struct yomi_entry **ye_array;
};

/* 汎用hash */
struct versatile_hash {
  char *buf;
  FILE *fp;
};

/* 辞書書き出し用の補助 */
void write_nl(FILE *fp, int i);

/* 用例辞書を作る */
struct uc_dict *read_uc_file(const char *fn, struct yomi_entry *ye);
void make_ucdic(FILE *out,  struct uc_dict *uc);
/**/

void fill_uc_to_hash(struct versatile_hash *vh, struct uc_dict *dict);

#endif

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
  /** 辞書ファイル中のオフセット */
  int offset;
};

/** ある読み */
struct yomi_entry {
  /* 読みの文字列 */
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
  /* 見出しのリスト */
  struct yomi_entry *head;
  /* 辞書ファイル中の見出しの数 */
  int nr_entries;
  /* 見出しの中で単語を持つものの数 */
  int nr_valid_entries;
  /* 単語の数 */
  int nr_words;
  /**/
  struct yomi_entry *hash[YOMI_HASH];
  struct yomi_entry **ye_array;
};

/* 汎用hash */
struct versatile_hash {
  char *buf;
  FILE *fp;
};

#define ADJUST_FREQ_UP 1
#define ADJUST_FREQ_DOWN 2
#define ADJUST_FREQ_KILL 3

/* 頻度補正用コマンド */
struct adjust_command {
  int type;
  xstr *yomi;
  char *wt;
  char *word;
  struct adjust_command *next;
};

/**/
struct yomi_entry *find_yomi_entry(struct yomi_entry_list *yl,
				   xstr *index, int create);

/* 辞書書き出し用の補助 */
void write_nl(FILE *fp, int i);

/* 用例辞書を作る */
struct uc_dict *create_uc_dict(struct yomi_entry_list *yl);
void read_uc_file(struct uc_dict *ud, const char *fn);
void make_ucdict(FILE *out,  struct uc_dict *uc);
/**/

void fill_uc_to_hash(struct versatile_hash *vh, struct uc_dict *dict);

#endif

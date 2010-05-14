#ifndef _dic_ent_h_included_
#define _dic_ent_h_included_

#include <wtype.h>
#include "dic.h"

/* 文字列のタイプ (seq_ent->seq_type) */
#define ST_NONE 0
/* Anthy的になんらかの意味のある文字列 */
#define ST_WORD 1
/* 辞書にのってない自立語 */
#define ST_UNKSEQ 2
/* 辞書にのってない非自立語、主にセパレータ */
#define ST_SEP 4
/**/
#define ST_REVERSE 8

/** ある単語 */
struct dic_ent {
  wtype_t type; /** 品詞 */
  const char *wt_name;
  int freq; /** 頻度 */
  xstr str; /** 変換結果の文字列 */
  /** 同一品詞の場合の辞書中の順番(anthy_get_seq_ent_wtype_freqから
      anthy_wtype_includeが呼ばれる回数を減らすのに用いる */
  int order;
};

/** 複合語のエントリ */
struct compound_ent {
  /* 品詞 */
  wtype_t type;
  /* 文字列(compound_entと同時に確保され、同時に解放される) */
  xstr *str;
  /* 頻度 */
  int freq;
};

/**ある文字列と同音異義語の配列
 * seq_ent_t として参照される
 */
struct seq_ent {
  xstr str;/* 読み */
  int mask;/* どのdic_sessionによって使用されているかのマスク */

  int seq_type; /** ST_(type) */
  int flags; /** ?F_* */

  /** dic_entの配列 */
  int nr_dic_ents;
  struct dic_ent **dic_ents;
  /** compound_entの配列 */
  int nr_compound_ents;
  struct compound_ent **compound_ents;

  /* 属するメモリ辞書 */
  struct mem_dic *md;
  /* メモリ辞書中のhash chain */
  struct seq_ent *next;
};

/* ext_ent.c */
void anthy_init_ext_ent(void);
/**/
int anthy_get_nr_dic_ents_of_ext_ent(struct seq_ent *se,xstr *xs);
int anthy_get_nth_dic_ent_str_of_ext_ent(seq_ent_t ,xstr *,int ,xstr *);
int anthy_get_nth_dic_ent_wtype_of_ext_ent(xstr *,int ,wtype_t *);
int anthy_get_nth_dic_ent_freq_of_ext_ent(struct seq_ent *se, int nth);
int anthy_get_ext_seq_ent_wtype(struct seq_ent *, wtype_t );
seq_ent_t anthy_get_ext_seq_ent_from_xstr(xstr *x, int is_reverse);
int anthy_get_ext_seq_ent_pos(struct seq_ent *, int);
int anthy_get_ext_seq_ent_indep(struct seq_ent *);
int anthy_get_ext_seq_ent_ct(struct seq_ent *, int, int);
int anthy_get_ext_seq_ent_wtype(struct seq_ent *se, wtype_t w);

#endif

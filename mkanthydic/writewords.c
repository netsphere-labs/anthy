/*
 * 読みから単語の情報を取得するデータ構造をファイル中に
 * 出力するためのコード
 *
 * データ構造を変更しやすくするためにmkdic.cから分離(2005/7/8)
 *
 * Copyright (C) 2000-2005 TABATA Yusuke
 */
#include <stdio.h>
#include <string.h>
#include <file_dic.h>
#include "mkdic.h"

extern FILE *page_out, *page_index_out;
extern FILE *yomi_entry_index_out, *yomi_entry_out;

/** 一つの読みに対する単語の内容を出力する */
static int
output_yomi_entry(struct yomi_entry *ye)
{
  int i;
  int count = 0;

  if (!ye) {
    return 0;
  }
  /* 各単語を出力する */
  for (i = 0; i < ye->nr_entries; i++) {
    struct word_entry *we = &ye->entries[i];
    /**/
    if (!we->freq) {
      continue;
    }
    if (i > 0) {
      /* 二つ目以降は空白から始まる */
      count += fprintf(yomi_entry_out, " ");
    }
    /* 品詞と頻度を出力する */
    if (i == 0 ||
	(strcmp(ye->entries[i-1].word, we->word) ||
	 strcmp(ye->entries[i-1].wt, we->wt) ||
	 ye->entries[i-1].freq != we->freq)) {
      count += fprintf(yomi_entry_out, "%s", we->wt);
      if (we->freq > 1) {
	count += fprintf(yomi_entry_out, "*%d", we->freq);
      }
      count += fprintf(yomi_entry_out, " ");
    }
    /* 単語を出力する場所がこの単語のid */
    we->offset = count + ye->offset;
    /* 単語を出力する */
    count += fprintf(yomi_entry_out, "%s", we->word);
  }

  fputc(0, yomi_entry_out);
  return count + 1;
}

/* 2つの文字列の共通部分の長さを求める */
static int
common_len(xstr *s1, xstr *s2)
{
  int m,i;
  if (!s1 || !s2) {
    return 0;
  }
  if (s1->len < s2->len) {
    m = s1->len;
  }else{
    m = s2->len;
  }
  for (i = 0; i < m; i++) {
    if (s1->str[i] != s2->str[i]) {
      return i;
    }
  }
  return m;
}

/*
 * 2つの文字列の差分を出力する
 * AAA ABBB という2つの文字列を見た場合には
 * ABBBはAAAのうしろ2文字を消してBBBを付けたものとして
 * \0x2BBBと出力される。
 */
static int
output_diff(xstr *p, xstr *c)
{
  int i, m, len = 1;
  m = common_len(p, c);
  if (p && p->len > m) {
    fprintf(page_out, "%c", p->len - m + 1);
  } else {
    fprintf(page_out, "%c", 1);
  }
  for (i = m; i < c-> len; i++) {
    char buf[8];
    len += anthy_sputxchar(buf, c->str[i], 0);
    fputs(buf, page_out);
  }
  return len;
}

static void
begin_new_page(int i)
{
  fputc(0, page_out);
  write_nl(page_index_out, i);
}

static void
output_entry_index(int i)
{
  write_nl(yomi_entry_index_out, i);
}

/** 単語辞書を出力する
 * また、このときに辞書中のオフセットも計算する */
void
output_word_dict(struct yomi_entry_list *yl)
{
  xstr *prev = NULL;
  int entry_index = 0;
  int page_index = 0;
  struct yomi_entry *ye = NULL;
  int i;

  /* まず、最初の読みに対するエントリのインデックスを書き出す */
  write_nl(page_index_out, page_index);

  /* 各読みに対するループ */
  for (i = 0; i < yl->nr_valid_entries; i++) {
    ye = yl->ye_array[i];
    /* 新しいページの開始 */
    if ((i % WORDS_PER_PAGE) == 0 && (i != 0)) {
      page_index ++;
      prev = NULL;
      begin_new_page(page_index);
    }

    /* 読みに対応する情報を出力する */
    page_index += output_diff(prev, ye->index_xstr);
    output_entry_index(entry_index);
    ye->offset = entry_index;
    entry_index += output_yomi_entry(ye);
    /***/
    prev = ye->index_xstr;
  }

  /* 最後の読みを終了 */
  entry_index += output_yomi_entry(ye);
  write_nl(yomi_entry_index_out, entry_index);
  write_nl(page_index_out, 0);

  /**/
  printf("Total %d indexes, %d words, (%d pages).\n",
	 yl->nr_valid_entries,
	 yl->nr_words,
	 yl->nr_valid_entries / WORDS_PER_PAGE + 1);
}

/*
 * 個人辞書管理用の関数群
 *
 * 互換性の都合で
 *  utf8の辞書はtextdic
 *  およびrecordを使ってて混乱しまくり
 * textdicへ移行する
 *
 * 開発予定
 *
 *  新規登録はtextdicに対して行うようにする <- todo
 *  record関係は消す
 *
 *
 * Funded by IPA未踏ソフトウェア創造事業 2001 10/24
 *
 * Copyright (C) 2001-2007 TABATA Yusuke
 *
 */
/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <anthy/anthy.h>
#include <anthy/conf.h>
#include <anthy/dic.h>
#include <anthy/textdic.h>
#include <anthy/dicutil.h>

#include "dic_main.h"
#include "dic_personality.h"

/* UTF8で32文字 x 3bytes */
#define MAX_KEY_LEN 96

static int gIsInit;
static int dic_util_encoding;

extern const char *anthy_private_text_dic;
/* 現在選択されている読み */
static struct iterate_contex {
  /* textdicの検索用 */
  int dicfile_offset;
  char *current_index;
  char *current_line;
} word_iterator;
/**/
struct scan_context {
  const char *yomi;
  const char *word;
  const char *wt_name;
  int offset;
  int found_word;
};

static void
set_current_line(const char *index, const char *line)
{
  if (word_iterator.current_line) {
    free(word_iterator.current_line);
    word_iterator.current_line = NULL;
  }
  if (line) {
    word_iterator.current_line = strdup(line);
  }
  if (word_iterator.current_index) {
    free(word_iterator.current_index);
    word_iterator.current_index = NULL;
  }
  if (index) {
    word_iterator.current_index = strdup(index);
  }
}

/** 個人辞書ライブラリを初期化する */
void
anthy_dic_util_init(void)
{
  if (gIsInit) {
    return ;
  }
  if (anthy_init_dic() == -1) {
    return ;
  }
  anthy_dic_set_personality("default");
  gIsInit = 1;
  dic_util_encoding = ANTHY_UTF8_ENCODING;
}

/** 辞書ライブラリを解放する */
void
anthy_dic_util_quit(void)
{
  if (gIsInit) {
    anthy_quit_dic();
  }
  set_current_line(NULL, NULL);
  gIsInit = 0;
}

/** 辞書ユーティリティAPIのエンコーディングを設定する */
int
anthy_dic_util_set_encoding(int enc)
{
  if (enc == ANTHY_UTF8_ENCODING ||
      enc == ANTHY_EUC_JP_ENCODING) {
    dic_util_encoding = enc;
  }
  return dic_util_encoding;
}

void
anthy_dic_util_set_personality(const char *id)
{
  anthy_dic_set_personality(id);
}

/** (API) 個人辞書を全部消す */
void
anthy_priv_dic_delete(void)
{
  while (!anthy_textdic_delete_line(anthy_private_text_dic, 0)) {
    /**/
  }
}

static int
scan_one_word_cb(void *p, long next_offset, const char *key, const char *n)
{
  (void)p;
  set_current_line(key, n);
  word_iterator.dicfile_offset = next_offset;
  return -1;
}

static int
select_first_entry_in_textdic(void)
{
  word_iterator.dicfile_offset = 0;
  set_current_line(NULL, NULL);
  anthy_textdic_scan(anthy_private_text_dic, word_iterator.dicfile_offset,
		     NULL, scan_one_word_cb);
  if (word_iterator.current_line) {
    return 0;
  }
  /* 単語が無い */
  return ANTHY_DIC_UTIL_ERROR;
}

/** (API) 最初の単語を選択する */
int
anthy_priv_dic_select_first_entry(void)
{
  return select_first_entry_in_textdic();
}

/** (API) 現在選択されている単語の次の単語を選択する */
int
anthy_priv_dic_select_next_entry(void)
{
  set_current_line(NULL, NULL);
  anthy_textdic_scan(anthy_private_text_dic, word_iterator.dicfile_offset,
		     NULL, scan_one_word_cb);
  if (word_iterator.current_line) {
    return 0;
  }
  return ANTHY_DIC_UTIL_ERROR;
}

/** 未実装 */
int
anthy_priv_dic_select_entry(const char *index)
{
  (void)index;
  return 0;
}

/** 現在選択されている単語の読みをを取得する */
char *
anthy_priv_dic_get_index(char *buf, int len)
{
  int i;
  char *src_buf;
  src_buf = word_iterator.current_index;
  if (dic_util_encoding == ANTHY_EUC_JP_ENCODING) {
    /**/
    src_buf = anthy_conv_utf8_to_euc(src_buf);
  } else {
    src_buf = strdup(src_buf);
  }
  /* 最初の空白か\0までをコピーする */
  for (i = 0; src_buf[i] && src_buf[i] != ' '; i++) {
    if (i >= len - 1) {
      free(src_buf);
      return NULL;
    }
    buf[i] = src_buf[i];
  }
  buf[i] = 0;
  free(src_buf);
  return buf;
}

/** 現在選択されている単語の頻度を取得する */
int
anthy_priv_dic_get_freq(void)
{
  struct word_line res;
  anthy_parse_word_line(word_iterator.current_line, &res);
  return res.freq;
}

/** 現在選択されている単語の品詞を取得する */
char *
anthy_priv_dic_get_wtype(char *buf, int len)
{
  struct word_line res;
  anthy_parse_word_line(word_iterator.current_line, &res);
  if (len - 1 < (int)strlen(res.wt)) {
    return NULL;
  }
  sprintf(buf, "%s", res.wt);
  return buf;
}

/** 現在選択されている単語を取得する */
char *
anthy_priv_dic_get_word(char *buf, int len)
{
  char *v;
  char *s;
  v = word_iterator.current_line;
  if (!v) {
    return NULL;
  }
  /* 品詞の後ろにある単語を取り出す */
  s = strchr(v, ' ');
  s++;
  if (dic_util_encoding == ANTHY_EUC_JP_ENCODING) {
    s = anthy_conv_utf8_to_euc(s);
    snprintf(buf, len, "%s", s);
    free(s);
  } else {
    snprintf(buf, len, "%s", s);
  }
  return buf;
}

static int
find_cb(void *p, long next_offset, const char *key, const char *n)
{
  struct scan_context *sc = p;
  struct word_line res;
  if (strcmp(key, sc->yomi)) {
    sc->offset = next_offset;
    return 0;
  }
  anthy_parse_word_line(n, &res);
  if (!strcmp(res.wt, sc->wt_name) &&
      !strcmp(res.word, sc->word)) {
    sc->found_word = 1;
    return -1;
  }
  sc->offset = next_offset;
  return 0;
}

static int
order_cb(void *p, long next_offset, const char *key, const char *n)
{
  struct scan_context *sc = p;
  (void)n;
  if (strcmp(key, sc->yomi) >= 0) {
    sc->found_word = 1;
    return -1;
  }
  sc->offset = next_offset;
  return 0;
}

/* 引数はutf8 */
static int
do_add_word_to_textdic(const char *td, int offset,
		       const char *yomi, const char *word,
		       const char *wt_name, int freq)
{
  char *buf = malloc(strlen(yomi) + strlen(word) + strlen(wt_name) + 20);
  int rv;
  if (!buf) {
    return -1;
  }
  sprintf(buf, "%s %s*%d %s\n", yomi, wt_name, freq, word);
  rv = anthy_textdic_insert_line(td, offset, buf);
  free(buf);
  return rv;
}

static int
add_word_to_textdic(const char *yomi, const char *word,
		    const char *wt_name, int freq)
{
  struct scan_context sc;
  int rv;
  int yomi_len = strlen(yomi);

  if (yomi_len > MAX_KEY_LEN || yomi_len == 0) {
    return ANTHY_DIC_UTIL_ERROR;
  }

  if (wt_name[0] != '#') {
    return ANTHY_DIC_UTIL_ERROR;
  }

  /* 同じ物があったら消す */
  sc.yomi = yomi;
  sc.word = word;
  sc.wt_name = wt_name;
  /**/
  sc.offset = 0;
  sc.found_word = 0;
  anthy_textdic_scan(anthy_private_text_dic, 0, &sc, find_cb);
  if (sc.found_word == 1) {
    anthy_textdic_delete_line(anthy_private_text_dic, sc.offset);
  }
  if (freq == 0) {
    return ANTHY_DIC_UTIL_OK;
  }
  /* 追加する場所を探す */
  sc.offset = 0;
  sc.found_word = 0;
  anthy_textdic_scan(anthy_private_text_dic, 0, &sc, order_cb);
  /* 追加する */
  rv = do_add_word_to_textdic(anthy_private_text_dic, sc.offset,
			      yomi, word, wt_name, freq);
  if (!rv) {
    return ANTHY_DIC_UTIL_OK;
  }
  return ANTHY_DIC_UTIL_ERROR;
}

/** 単語を登録する
 * 頻度が0の場合は削除
 */
int
anthy_priv_dic_add_entry(const char *yomi, const char *word,
			 const char *wt_name, int freq)
{
  if (dic_util_encoding == ANTHY_UTF8_ENCODING) {
    return add_word_to_textdic(yomi, word, wt_name, freq);
  } else {
    int rv;
    char *yomi_utf8 = anthy_conv_euc_to_utf8(yomi);
    char *word_utf8 = anthy_conv_euc_to_utf8(word);
    rv = add_word_to_textdic(yomi_utf8, word_utf8, wt_name, freq);
    free(yomi_utf8);
    free(word_utf8);
    return rv;
  }
}

const char *
anthy_dic_util_get_anthydir(void)
{
  return anthy_conf_get_str("ANTHYDIR");
}

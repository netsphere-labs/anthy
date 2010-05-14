/*
 * 個人辞書管理用の関数群
 *
 * Funded by IPA未踏ソフトウェア創造事業 2001 10/24
 *
 * Copyright (C) 2001-2004 TABATA Yusuke
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

#include <anthy.h>
#include <conf.h>
#include <dic.h>
#include <record.h>
#include <dicutil.h>

static int gIsInit;

/* record中の同じindexの中の何番目から値を取り出すか */
static int gOffset;

static int dic_util_encoding;

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
  dic_util_encoding = ANTHY_EUC_JP_ENCODING;
}

/** 辞書ライブラリを解放する */
void
anthy_dic_util_quit(void)
{
  if (gIsInit) {
    anthy_quit_dic();
  }
}

/** 辞書ユーティリティAPIのエンコーディングを設定する */
int
anthy_dic_util_set_encoding(int enc)
{
#ifdef USE_UCS4
  if (enc == ANTHY_EUC_JP_ENCODING ||
      enc == ANTHY_UTF8_ENCODING) {
    dic_util_encoding = enc;
  }
  return dic_util_encoding;
#else
  (void)enc;
  return ANTHY_EUC_JP_ENCODING;
#endif
}

void
anthy_dic_util_set_personality(const char *id)
{
  anthy_dic_set_personality(id);
}

/** 個人辞書を全部消す */
void
anthy_priv_dic_delete(void)
{
  if (anthy_select_section("PRIVATEDIC", 0) == -1) {
    return ;
  }
  /* sectionの内容を破棄する */
  anthy_release_section();
}

/** 最初の単語を選択する */
int
anthy_priv_dic_select_first_entry(void)
{
  if (anthy_select_section("PRIVATEDIC", 0) == -1) {
    return -1;
  }
  gOffset = 0;
  return anthy_select_first_column();
}

/** 現在選択されている単語の次の単語を選択する */
int
anthy_priv_dic_select_next_entry(void)
{
  int nr = anthy_get_nr_values();
  if (nr > gOffset + 3) {
    gOffset += 3;
    return 0;
  }
  gOffset = 0;
  return anthy_select_next_column();
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
  xstr *xs;
  xs = anthy_get_index_xstr();
  if (!xs) {
    return NULL;
  }
  if (anthy_snputxstr(buf, len, xs, dic_util_encoding) == len) {
    return NULL;
  }
  return buf;
}

/** 現在選択されている単語の読みを取得する */
int
anthy_priv_dic_get_freq(void)
{
  return anthy_get_nth_value(gOffset + 2);
}

/** 現在選択されている単語の品詞を取得する */
char *
anthy_priv_dic_get_wtype(char *buf, int len)
{
  xstr *xs = anthy_get_nth_xstr(gOffset + 1);
  if (!xs) {
    return 0;
  }
  if (anthy_snputxstr(buf, len, xs, dic_util_encoding) == len) {
    return 0;
  }
  return buf;
}

/** 現在選択されている単語を取得する */
char *
anthy_priv_dic_get_word(char *buf, int len)
{
  xstr *xs = anthy_get_nth_xstr(gOffset);
  if (!xs) {
    return 0;
  }
  if (anthy_snputxstr(buf, len, xs, dic_util_encoding) == len) {
    return 0;
  }
  return buf;
}

/* 単語を登録する場所を計算する */
static int
get_offset(xstr *word, xstr *wt)
{
  int i, nr = anthy_get_nr_values();
  for (i = 0; i < nr; i+= 3) {
    xstr *xs;
    xs = anthy_get_nth_xstr(i);
    if (!anthy_xstrcmp(word, xs)) {
      xs = anthy_get_nth_xstr(i + 1);
      if (!anthy_xstrcmp(wt, xs)) {
	return i;
      }
    }
  }
  return nr;
}

/** 行をshrinkする/頻度0のものを消す
 */
static void
shrink_column(void)
{
  int nr, i, nth, res;
  struct priv_ent {
    xstr *word_xs;
    xstr *wt_xs;
    int freq;
  } * priv_array;
  xstr *index = anthy_get_index_xstr();

  nr = anthy_get_nr_values() / 3;
  priv_array = alloca(sizeof(struct priv_ent) * nr);
  nth = 0;
  for (i = 0; i < nr; i++) {
    int freq = anthy_get_nth_value(i * 3 + 2);
    if (freq) {
      priv_array[nth].word_xs = anthy_get_nth_xstr(i * 3);
      priv_array[nth].wt_xs = anthy_get_nth_xstr(i * 3 + 1);
      priv_array[nth].freq = freq;
      nth ++;
    }
  }
  if (nth == nr) {
    /* 頻度0のものは無かった */
    return ;
  }
  if (!nth) {
    /* 一個もなかったので、columnごと削除 */
    anthy_release_column();
    return ;
  }
  /* columnを解放して作りなおす */
  index = anthy_xstr_dup(index);
  anthy_release_column();
  res = anthy_select_column(index, 1);
  if (res == -1) {
    anthy_free_xstr(index);
    return ;
  }
  for (i = 0; i < nth; i++) {
    anthy_set_nth_xstr(nth * 3, priv_array[nth].word_xs);
    anthy_set_nth_xstr(nth * 3 + 1, priv_array[nth].wt_xs);
    anthy_set_nth_value(nth * 3 + 2, priv_array[nth].freq);
  }
  anthy_free_xstr(index);
}

/** 単語を登録する */
int
anthy_priv_dic_add_entry(const char *yomi, const char *word,
			 const char *wt_name, int freq)
{
  xstr *xs, *word_xs, *wt_xs;
  int offset, res;

  if (anthy_select_section("PRIVATEDIC", 1) == -1) {
    return ANTHY_DIC_UTIL_ERROR;
  }
  /* 読みを選択する*/
  xs = anthy_cstr_to_xstr(yomi, 0);
  if (anthy_select_column(xs, 1) == -1) {
    anthy_free_xstr(xs);
    return ANTHY_DIC_UTIL_ERROR;
  }
  anthy_free_xstr(xs);

  /* 同じ単語がすでにあったらそこに上書き、なければ最後 */
  word_xs = anthy_cstr_to_xstr(word, 0);
  wt_xs = anthy_cstr_to_xstr(wt_name, 0);
  offset = get_offset(word_xs, wt_xs);
  if (offset < anthy_get_nr_values()) {
    res = ANTHY_DIC_UTIL_DUPLICATE;
  } else {
    res = ANTHY_DIC_UTIL_OK;
  }
  /* 単語を登録する */
  anthy_set_nth_xstr(offset, word_xs);
  anthy_free_xstr(word_xs);
  /* 品詞を登録する */
  anthy_set_nth_xstr(offset + 1, wt_xs);
  anthy_free_xstr(wt_xs);
  /* 頻度 */
  anthy_set_nth_value(offset + 2, freq);
  anthy_mark_column_used();
  /**/
  shrink_column();

  return res;
}

const char *
anthy_dic_util_get_anthydir(void)
{
  return anthy_conf_get_str("ANTHYDIR");
}

/**/
static char *
do_search(FILE *fp, const char *word)
{
  char buf[32];
  char *res = NULL;
  int word_len = strlen(word);
  while (fgets(buf, 32, fp)) {
    int len = strlen(buf);
    buf[len - 1] = 0;
    len --;
    if (len > word_len) {
      continue;
    }
    if (!strncasecmp(buf, word, len)) {
      if (res) {
	free(res);
      }
      res = strdup(buf);
    }
  }
  return res;
}

char *
anthy_dic_search_words_file(const char *word)
{
  FILE *fp;
  char *res;
  const char *words_dict_fn = anthy_conf_get_str("WORDS_FILE");
  if (!words_dict_fn) {
    return NULL;
  }
  fp = fopen(words_dict_fn, "r");
  if (!fp) {
    return NULL;
  }
  res = do_search(fp, word);
  fclose(fp);
  return res;
}

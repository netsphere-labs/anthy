/*
 * 個人辞書管理用の関数群
 *
 * Funded by IPA未踏ソフトウェア創造事業 2001 10/24
 *
 * Copyright (C) 2001-2006 TABATA Yusuke
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
#include <texttrie.h>
#include <record.h>
#include <dicutil.h>

#include "dic_main.h"
#include "dic_personality.h"

/*
 * 個人辞書はtexttrie中に格納されるとき
 * 「  見出し 数字」 -> 「#品詞*頻度 単語」という形式をとる
 * 最初の2文字の空白は単語情報のセクションであることを意味し、
 * 数字の部分は同音語を区別するために用いられる。
 *
 */

/* UTF8で32文字 x 3bytes */
#define MAX_KEY_LEN 96

static int gIsInit;
static int dic_util_encoding;

/* record中の同じindexの中の何番目から値を取り出すか */
extern struct text_trie *anthy_private_tt_dic;
/* 現在選択されている読み */
static char key_buf[MAX_KEY_LEN+32];

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
  /**/
  key_buf[0] = 0;
}

/** 辞書ライブラリを解放する */
void
anthy_dic_util_quit(void)
{
  if (gIsInit) {
    anthy_quit_dic();
  }
  gIsInit = 0;
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

static char *
find_next_key(void)
{
  char *v;
  v = anthy_trie_find_next_key(anthy_private_tt_dic,
			       key_buf, MAX_KEY_LEN+32);

  if (v && v[0] == ' ' && v[1] == ' ') {
    return v;
  }
  /**/
  sprintf(key_buf, "  ");
  return NULL;
}

/** 個人辞書を全部消す */
void
anthy_priv_dic_delete(void)
{
  sprintf(key_buf, "  ");
  anthy_priv_dic_lock();
  /* key_bufが"  "であれば、find_next_key()は
     最初の単語を返す */
  while (find_next_key()) {
    anthy_trie_delete(anthy_private_tt_dic, key_buf);
    sprintf(key_buf, "  ");
  }
  anthy_priv_dic_unlock();
  /* 旧形式の辞書も削除する */
  if (!anthy_select_section("PRIVATEDIC", 0)) {
    anthy_release_section();
    return ;
  }
}

/** 最初の単語を選択する */
int
anthy_priv_dic_select_first_entry(void)
{
  if (!anthy_private_tt_dic) {
    return ANTHY_DIC_UTIL_INVALID;
  }
  sprintf(key_buf, "  ");
  /* "  "の次のエントリが最初のエントリ */
  if (find_next_key()) {
    return 0;
  }
  return ANTHY_DIC_UTIL_ERROR;
}

/** 現在選択されている単語の次の単語を選択する */
int
anthy_priv_dic_select_next_entry(void)
{
  if (find_next_key()) {
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
  char *src_buf = &key_buf[2];
  /* 最初の空白か\0までをコピーする */
  for (i = 0; src_buf[i] && src_buf[i] != ' '; i++) {
    if (i >= len - 1) {
      return NULL;
    }
    buf[i] = src_buf[i];
  }
  buf[i] = 0;
  return buf;
}

/** 現在選択されている単語の頻度を取得する */
int
anthy_priv_dic_get_freq(void)
{
  struct word_line res;
  char *v = anthy_trie_find(anthy_private_tt_dic, key_buf);
  anthy_parse_word_line(v, &res);
  free(v);
  return res.freq;
}

/** 現在選択されている単語の品詞を取得する */
char *
anthy_priv_dic_get_wtype(char *buf, int len)
{
  struct word_line res;
  char *v = anthy_trie_find(anthy_private_tt_dic, key_buf);
  anthy_parse_word_line(v, &res);
  free(v);
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
  char *v = anthy_trie_find(anthy_private_tt_dic, key_buf);
  char *s;
  if (!v) {
    return NULL;
  }
  s = strchr(v, ' ');
  s++;
  snprintf(buf, len, "%s", s);
  free(v);
  return buf;
}

static int
dup_word_check(const char *v, const char *word, const char *wt)
{
  struct word_line res;

  anthy_parse_word_line(v, &res);

  /* 読みと単語を比較する */
  if (!strcmp(res.wt, wt) &&
      !strcmp(res.word, word)) {
    return 1;
  }
  return 0;
}

static void
do_add_entry(const char *yomi, const char *word,
	     const char *wt_name, int freq)
{
  int i = 0, ok = 0;
  char *cur;
  char word_buf[256];
  char *idx_buf = malloc(strlen(yomi) + 12);
  do {
    sprintf(idx_buf, "  %s %d", yomi, i);
    cur = anthy_trie_find(anthy_private_tt_dic, idx_buf);
    if (cur) {
      free(cur);
    } else {
      ok = 1;
    }
    i++;
  } while (!ok);
  sprintf(word_buf, "%s*%d %s", wt_name, freq, word);
  anthy_trie_add(anthy_private_tt_dic, idx_buf, word_buf);
  free(idx_buf);
}

static int
find_same_word(char *idx_buf, const char *yomi,
	       const char *word, const char *wt_name, int yomi_len)
{
  int found = 0;
  sprintf(idx_buf, "  %s ", yomi);
  anthy_trie_find_next_key(anthy_private_tt_dic,
			   idx_buf, yomi_len + 12);

  /* trieのインデックスを探す */
  do {
    char *v;
    if (strncmp(&idx_buf[2], yomi, yomi_len) ||
	idx_buf[yomi_len+2] != ' ') {
      /* 見出語が異なるのでループ終了 */
      break;
    }
    /* texttrieにアクセスして、見出語以外も一致しているかをチェック */
    v = anthy_trie_find(anthy_private_tt_dic, idx_buf);
    if (v) {
      found = dup_word_check(v, word, wt_name);
      free(v);
      if (found) {
	break;
      }
    }
  } while (anthy_trie_find_next_key(anthy_private_tt_dic,
				    idx_buf, yomi_len + 12));

  return found;
}

/** 単語を登録する
 * 頻度が0の場合は削除
 */
int
anthy_priv_dic_add_entry(const char *yomi, const char *word,
			 const char *wt_name, int freq)
{
  int yomi_len = strlen(yomi);
  char *idx_buf;
  int found = 0;
  int rv = ANTHY_DIC_UTIL_OK;

  if (!anthy_private_tt_dic) {
    return ANTHY_DIC_UTIL_ERROR;
  }

  if (yomi_len > MAX_KEY_LEN) {
    return ANTHY_DIC_UTIL_ERROR;
  }

  if (wt_name[0] != '#') {
    return ANTHY_DIC_UTIL_ERROR;
  }

  /* 同じ見出しの語があるかチェックする */
  idx_buf = malloc(yomi_len + 12);
  found = find_same_word(idx_buf, yomi, word, wt_name, yomi_len);

  if (freq > 0) {
    /* 登録 */
    if (found) {
      /* 既存のものを削除 */
      anthy_trie_delete(anthy_private_tt_dic, idx_buf);
      rv = ANTHY_DIC_UTIL_DUPLICATE;
    }
    do_add_entry(yomi, word, wt_name, freq);
  } else if (found) {
    /* 頻度が正ではないので、削除 */
    anthy_trie_delete(anthy_private_tt_dic, idx_buf);
  }
  free(idx_buf);
  return rv;
}

const char *
anthy_dic_util_get_anthydir(void)
{
  return anthy_conf_get_str("ANTHYDIR");
}

/* lookコマンドの辞書を検索するための関数 */
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

/* lookコマンドの辞書を検索するAPI */
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

/*
 * Word Dictionary
 * ファイルの辞書のインターフェース、存在するデータは
 * キャッシュされるのでここでは存在しない単語の
 * サーチを高速にする必要がある。
 *
 * anthy_word_dic_fill_seq_ent_by_xstr()が中心となる関数である
 *  指定したword_dicから指定した文字列をインデックスとしてもつエントリに
 *  語尾を付加してseq_entに追加する
 *
 * a)辞書の形式とb)辞書アクセスの高速化c)辞書ファイルのエンコーディング
 *  このソース中で扱ってるのでかなり複雑化してます．
 *
 * Copyright (C) 2000-2006 TABATA Yusuke
 * Copyright (C) 2005-2006 YOSHIDA Yuichi
 * Copyright (C) 2001-2002 TAKAI Kosuke
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include <alloc.h>
#include <dic.h>
#include <word_dic.h>
#include <logger.h>
#include <xstr.h>
#include <diclib.h>

#include "dic_main.h"
#include "dic_ent.h"

static allocator word_dic_ator;

/* 1バイト目を見て、文字が何バイトあるかを返す */
static int
mb_fragment_len(char *str)
{
#ifdef USE_UCS4
  unsigned char *tmp = (unsigned char *)str;
  if (*tmp < 0x80) {
    return 1;
  } else if (*tmp < 0xe0) {
    return 2;
  } else if (*tmp < 0xf0) {
    return 3;
  } else if (*tmp < 0xf8) {
    return 4;
  } else if (*tmp < 0xfc) {
    return 5;
  } else {
    return 6;
  }
#else
  if ((*str) & 0x80) {
    return 2;
  }
#endif
  return 1;
}

static int
is_printable(char *str)
{
  unsigned char *tmp = (unsigned char *)str;
  if (*tmp > 31 && *tmp < 127) {
    return 1;
  }
  if (mb_fragment_len(str) > 1) {
    return 1;
  }
  return 0;
}

/* 辞書のエンコーディングからxcharを作る */
static xchar
form_mb_char(char *str)
{
#ifdef USE_UCS4
  xchar res;
  anthy_utf8_to_ucs4_xchar(str, &res);
  return res;
#else
  unsigned char *tmp = (unsigned char *)str;
  return tmp[0] * 256 + tmp[1];
#endif
}

static int
hash(xstr *x)
{
  return anthy_xstr_hash(x)&
    (YOMI_HASH_ARRAY_SIZE*YOMI_HASH_ARRAY_BITS-1);
}

static int
check_hash_ent(struct word_dic *wdic, xstr *xs)
{
  int val = hash(xs);
  int idx = (val>>YOMI_HASH_ARRAY_SHIFT)&(YOMI_HASH_ARRAY_SIZE-1);
  int bit = val & ((1<<YOMI_HASH_ARRAY_SHIFT)-1);
  return wdic->hash_ent[idx] & (1<<bit);
}

static int
wtype_str_len(char *str)
{
  int i;
  for (i = 0; str[i] && str[i]!= ' '; i++);
  return i;
}

/* 辞書の行中をスキャンするための状態保持 */
struct wt_stat {
  wtype_t wt;
  const char *wt_name;
  int freq;
  int order_bonus;/* 辞書中の順序による頻度のボーナス */
  int offset;/* 文字列中のオフセット */
  char *line;
};
/*
 * #XX*123 というCannadicの形式をパーズする
 */
static const char *
parse_wtype_str(struct wt_stat *ws)
{
  int len;
  char *buf;
  char *freq_part;
  const char *wt_name;
  /* バッファへコピーする */
  len = wtype_str_len(&ws->line[ws->offset]);
  buf = alloca(len + 1);
  strncpy(buf, &ws->line[ws->offset], len);
  buf[len] = 0;

  /* parseする */
  freq_part = strchr(buf, '*');
  if (freq_part) {
    *freq_part = 0;
    freq_part ++;
    ws->freq = atoi(freq_part) * FREQ_RATIO;
  } else {
    ws->freq = FREQ_RATIO - 2;
  }

  wt_name = anthy_type_to_wtype(buf, &ws->wt);
  if (!wt_name) {
    ws->wt = anthy_wt_none;
  }
  ws->offset += len;
  return wt_name;
}


static int
normalize_freq(struct wt_stat* ws)
{
  if (ws->freq < 0) {
    ws->freq *= -1;
  }
  return ws->freq + ws->order_bonus;
}

/* '\\'によるエスケープに対応したコピー */
static void
copy_to_buf(char *buf, const char *src, int char_count)
{
  int pos;
  int i;
  pos = 0;
  for (i = 0; i < char_count; i++){
    if (src[pos] == '\\') {
      if (src[pos + 1] == ' ') {
	pos ++;
      } else if (src[pos + 1] == '\\') {
	pos ++;
      }
    }
    buf[i] = src[pos];
    pos ++;
  }
  buf[pos] = 0;
}

/** seq_entにdic_entを追加する */
static int
add_dic_ent(struct seq_ent *seq, struct wt_stat *ws,
	    xstr* yomi, int is_reverse)
{
  int i;
  /* 辞書ファイル中のバイト数 */
  int char_count;
  char *buf;
  xstr *xs;
  int freq;
  wtype_t w = ws->wt;
  char *s = &ws->line[ws->offset];

  /* 単語の文字数を計算 */
  for (i = 0, char_count = 0;
       s[i] && (s[i] != ' ') && (s[i] != '#'); i++) {
    char_count ++;
    if (s[i] == '\\') {
      char_count++;
      i++;
    }
  }

  /* 品詞が定義されていないので無視 */
  if (!ws->wt_name) {
    return char_count;
  }

  /* freqが負なのは逆変換用 */
  if (!is_reverse && ws->freq < 0) {
    return char_count;
  }

  /* bufに単語をコピー */
  buf = alloca(char_count+1);
  copy_to_buf(buf, s, char_count);

  xs = anthy_cstr_to_xstr(buf, 0);

  /* freqが正なのは順変換用 */
  if (is_reverse && ws->freq > 0) {
    /* 再変換の際に、変換済みの部分と未変換の部分が混じっていた場合に対応する為に、
       平仮名のみからなる部分は順辞書にその読みを持つ単語があればdic_entを生成する。
    */
    if (anthy_get_xstr_type(yomi) & XCT_HIRA) {
      freq = normalize_freq(ws);
      anthy_mem_dic_push_back_dic_ent(seq, yomi, w, ws->wt_name, freq);
    }      
    anthy_free_xstr(xs);
    return char_count;
  }

  freq = normalize_freq(ws);

  anthy_mem_dic_push_back_dic_ent(seq, xs, w, ws->wt_name, freq);
  if (anthy_wtype_get_meisi(w)) {
    /* 連用形が名詞化するやつは名詞化したものも追加 */
    w = anthy_get_wtype_with_ct(w, CT_MEISIKA);
    anthy_mem_dic_push_back_dic_ent(seq, xs, w, ws->wt_name, freq);
  }
  anthy_free_xstr(xs);
  return char_count;
}

static int
add_compound_ent(struct seq_ent *seq, struct wt_stat *ws,
		 xstr* yomi,
		 int is_reverse)
{
  int len = wtype_str_len(&ws->line[ws->offset]);
  char *buf = alloca(len);
  xstr *xs;
  int freq;

  (void)yomi;

  /* freqが負なのは逆変換用 */
  if (!is_reverse && ws->freq < 0) {
    /* 普段の変換では要らない */
    return len;
  }

  /* freqが正なのは順変換用 */
  if (is_reverse && ws->freq > 0) {

    /* 再変換の際に、変換済みの部分と未変換の部分が混じっていた場合に対応する為に、
       平仮名のみからなる部分は順辞書にその読みを持つ単語があればdic_entを生成する。
    */
    /*
      yomiに#_等を付加した文字列を作る必要がある
    if (anthy_get_xstr_type(yomi) & (XCT_HIRA | XCT_KATA)) {
      freq = normalize_freq(ws);
      anthy_mem_dic_push_back_compound_ent(seq, xs, ws->wt, freq);
    }
    */
    return len;
  }

  strncpy(buf, &ws->line[ws->offset + 1], len - 1);
  buf[len - 1] = 0;
  xs = anthy_cstr_to_xstr(buf, 0);

  freq = normalize_freq(ws);
  anthy_mem_dic_push_back_compound_ent(seq, xs, ws->wt, freq);

  return len;
}

/** 辞書のエントリの情報を元にseq_entをうめる */
void
anthy_fill_dic_ent(char *dic ,int idx, struct seq_ent *seq, 
		   xstr* yomi, int is_reverse)
{
  struct wt_stat ws;

  ws.wt_name = NULL;
  ws.freq = 0;
  ws.order_bonus = 0;
  ws.offset = 0;
  ws.line = &dic[idx];

  while (ws.line[ws.offset]) {
    if (ws.line[ws.offset] == '#') {
      if (isalpha(ws.line[ws.offset + 1])) {
	/* 品詞*頻度 */
	ws.wt_name = parse_wtype_str(&ws);
	/**/
	ws.order_bonus = FREQ_RATIO - 1;
      } else {
	/* 複合語候補 */
	ws.offset += add_compound_ent(seq, &ws,
				      yomi,
				      is_reverse);
      }
    } else {
      /* 単語 */
      ws.offset += add_dic_ent(seq, &ws, yomi,
			       is_reverse);
      if (ws.order_bonus > 0) {
	ws.order_bonus --;
      }
    }
    if (ws.line[ws.offset] == ' ') {
      ws.offset++;
    }
  }
}

/*
 * sに書かれた文字列によってxを変更する
 * 返り値は読み進めたバイト数
 */
static int
mkxstr(char *s, xstr *x)
{
  int i, len;
  /* s[0]には巻き戻しの文字数 */
  x->len -= (s[0] - 1);
  for (i = 1; is_printable(&s[i]); i ++) {
    len = mb_fragment_len(&s[i]);
    if (len > 1) {
      /* マルチバイト */
      x->str[x->len] = form_mb_char(&s[i]);
      x->len ++;
      i += (len - 1);
    } else {
      /* 1バイト文字 */
      x->str[x->len] = s[i];
      x->len ++;
    }
  } 
  return i;
}

/** ページ中の単語の場所を調べる */
static int
search_word_in_page(xstr *x, char *s)
{
  int o = 0;
  xchar *buf;
  xstr xs;
  /* このページ中にあるもっとも長い単語を格納しうる長さ */
  buf = alloca(sizeof(xchar)*strlen(s)/2);
  xs.str = buf;
  xs.len = 0;

  while (*s) {
    s += mkxstr(s, &xs);
    if (!anthy_xstrcmp(&xs, x)) {
      return o;
    }
    o ++;
  }
  return -1;
}

/**/
static int
compare_page_index(struct word_dic *wdic, const char *key, int page)
{
  char buf[100];
  char *s = &wdic->page[anthy_dic_ntohl(wdic->page_index[page])];
  int i;
  s++;
  for (i = 0; is_printable(&s[i]);) {
    int j, l = mb_fragment_len(&s[i]);
    for (j = 0; j < l; j++) {
      buf[i+j] = s[i+j];
    }
    i += l;
  }
  buf[i] = 0;
  return strcmp(key ,buf);
}

/* バイナリサーチをする */
static int
get_page_index_search(struct word_dic *wdic, const char *key, int f, int t)
{
  /* anthy_xstrcmpが-1で無くなったところを探す */
  int c,p;
  c = (f+t)/2;
  if (f+1==t) {
    return c;
  } else {
    p = compare_page_index(wdic, key, c);
    if (p < 0) {
      return get_page_index_search(wdic, key, f, c);
    } else {
      /* c<= <t */
      return get_page_index_search(wdic, key, c, t);
    }
  } 
}

/** keyを含む可能性のあるページの番号を得る、
 * 範囲チェックをしてバイナリサーチを行うget_page_index_searchを呼ぶ
 */
static int
get_page_index(struct word_dic *wdic, const char *key)
{
  int page;
  /* 最初のページの読みよりも小さい */
  if (compare_page_index(wdic, key, 0) < 0) {
    return -1;
  }
  /* 最後のページの読みよりも大きいので、最後のページに含まれる可能性がある */
  if (compare_page_index(wdic, key, wdic->nr_pages-1) >= 0) {
    return wdic->nr_pages-1;
  }
  /* バイナリサーチ */
  page = get_page_index_search(wdic, key, 0, wdic->nr_pages);
  return page;
}

static int
get_nr_page(struct word_dic *h)
{
  int i;
  for (i = 1; anthy_dic_ntohl(h->page_index[i]); i++);
  return i;
}

static char *
get_section(struct word_dic *wdic, int section)
{
  int *p = (int *)wdic->dic_file;
  int offset = anthy_dic_ntohl(p[section]);
  return &wdic->dic_file[offset];
}

/** 辞書ファイルをmmapして、word_dic中の各セクションのポインタを取得する */
static int
get_word_dic_sections(struct word_dic *wdic)
{
  wdic->entry_index = (int *)get_section(wdic, 2);
  wdic->entry = (char *)get_section(wdic, 3);
  wdic->page = (char *)get_section(wdic, 4);
  wdic->page_index = (int *)get_section(wdic, 5);
  wdic->uc_section = (char *)get_section(wdic, 6);
  wdic->hash_ent = (unsigned char *)get_section(wdic, 7);

  return 0;
}

/** 指定された単語の辞書中のインデックスを調べる */
static int
search_yomi_index(struct word_dic *wdic, xstr *xs)
{
  int p, o;
  int page_number;
  char *key = anthy_xstr_to_cstr(xs, 0);

  p = get_page_index(wdic, key);
  free(key);
  if (p == -1) {
    return -1;
  }

  page_number = anthy_dic_ntohl(wdic->page_index[p]);
  o = search_word_in_page(xs, &wdic->page[page_number]);

  if (o == -1) {
    return -1;
  }
  return o + p * WORDS_PER_PAGE;
}

/** word_dicから単語を検索する
 * 辞書キャッシュから呼ばれる
 * 指定された活用の単語をword_dicから探し，tailを付加して
 * mem_dic_push_back_dic_entを用いて，seq_entに追加する．
 */
void
anthy_word_dic_fill_seq_ent_by_xstr(struct word_dic *wdic, xstr *xs,
				    struct seq_ent *se,
				    int is_reverse)
{
  int yomi_index;

  if (xs->len > 31) {
    /* 32文字以上単語には未対応 */
    return;
  }
  /* hashにないなら除去 */
  if (!check_hash_ent(wdic, xs)) {
    return;
  }
  yomi_index = search_yomi_index(wdic, xs);

  if (yomi_index >= 0) {
    /* 該当する読みが辞書中にあれば、それを引数にseq_entを埋める */
    int entry_index = anthy_dic_ntohl(wdic->entry_index[yomi_index]);
    se->seq_type |= ST_WORD;
    anthy_fill_dic_ent(wdic->entry,
		       entry_index,
		       se,
		       xs,
		       is_reverse);

  }
}

char *
anthy_word_dic_get_hashmap_ptr(struct word_dic *wdic)
{
  return get_section(wdic, 8);
}

struct word_dic *
anthy_create_word_dic(void)
{
  struct word_dic *wdic;
  char *p;

  wdic = anthy_smalloc(word_dic_ator);
  memset(wdic, 0, sizeof(*wdic));

  /* 辞書ファイルをマップする */
  wdic->dic_file = anthy_file_dic_get_section("word_dic");

  /* 単語辞書のhashを構成する */
  if (get_word_dic_sections(wdic) == -1) {
    anthy_sfree(word_dic_ator, wdic);
    return 0;
  }
  wdic->nr_pages = get_nr_page(wdic);

  /* 用例辞書をマップする */
  p = wdic->uc_section;
  return wdic;
}

void
anthy_release_word_dic(struct word_dic *wdic)
{
  anthy_sfree(word_dic_ator, wdic);
}

void
anthy_init_word_dic(void)
{
  word_dic_ator = anthy_create_allocator(sizeof(struct word_dic), NULL);
}

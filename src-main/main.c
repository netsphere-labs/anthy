/*
 * Comments in this program are written in Japanese,
 * because this program is a Japanese input method.
 * (many Japanese gramatical terms will appear.)
 *
 * Kana-Kanji conversion engine Anthy.
 * 仮名漢字変換エンジンAnthy(アンシー)
 *
 * Funded by IPA未踏ソフトウェア創造事業 2001 9/22
 * Copyright (C) 2000-2005 TABATA Yusuke, UGAWA Tomoharu
 * Copyright (C) 2004-2005 YOSHIDA Yuichi
 * Copyright (C) 2000-2005 KMC(Kyoto University Micro Computer Club)
 * Copyright (C) 2001-2002 TAKAI Kosuke, Nobuoka Takahiro
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
/*
 * Anthyの変換機能はライブラリとして構成されており、この
 * ファイルにはライブラリの提供する関数(API)が記述されています。
 *
 * ライブラリの提供する関数は下記のようなものがあります
 * (1)ライブラリ全体の初期化、終了、設定
 * (2)変換コンテキストの作成、解放
 * (3)変換コンテキストに対する文字列の設定、文節長の変更、候補の取得等
 *
 * インターフェイスに関しては doc/LIBを参照してください
 * Anthyのコードを理解しようとする場合は
 * doc/GLOSSARY で用語を把握することを勧めます
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <dic.h>
#include <splitter.h>
#include <conf.h>
#include <ordering.h>
#include <logger.h>
#include <record.h>
#include <anthy.h>
#include "main.h"
#include <config.h>

/** Anthyの初期化が完了したかどうかのフラグ */
static int is_init_ok;

static int default_encoding;

/** (API) 全体の初期化 */
int
anthy_init(void)
{
  if (is_init_ok) {
    /* 2度初期化しないように */
    return 0;
  }

  /* 各サブシステムを順に初期化する */
  if (anthy_init_dic()) {
    anthy_log(0, "Failed to open dictionary.\n");
    return -1;
  }

  if (anthy_init_splitter()) {
    anthy_log(0, "Failed to init splitter.\n");
    return -1;
  }
  anthy_init_contexts();
  anthy_init_personality();

  default_encoding = ANTHY_EUC_JP_ENCODING;
  is_init_ok = 1;
  return 0;
}

/** (API) 全データの解放 */
void
anthy_quit(void)
{
  if (!is_init_ok) {
    return ;
  }
  anthy_quit_contexts();
  anthy_quit_personality();
  anthy_quit_splitter();
  /* 多くのデータ構造はここでallocatorによって解放される */
  anthy_quit_dic();
  is_init_ok = 0;
}

/** (API) 設定項目の上書き */
void
anthy_conf_override(const char *var, const char *val)
{
  anthy_do_conf_override(var, val);
}

/** (API) personalityの設定 */
int
anthy_set_personality(const char *id)
{
  return anthy_do_set_personality(id);
}

/** (API) 変換contextの作成 */
struct anthy_context *
anthy_create_context(void)
{
  if (!is_init_ok) {
    return 0;
  }
  return anthy_do_create_context(default_encoding);
}

/** (API) 変換contextのリセット */
void
anthy_reset_context(struct anthy_context *ac)
{
  anthy_do_reset_context(ac);
}

/** (API) 変換contextの解放 */
void
anthy_release_context(struct anthy_context *ac)
{
  anthy_do_release_context(ac);
}

/** (API) 変換文字列の設定 */
int
anthy_set_string(struct anthy_context *ac, const char *s)
{
  xstr *xs;
  int retval;

  anthy_dic_activate_session(ac->dic_session);
  /* 変換を開始する前に個人辞書をreloadする */
  anthy_reload_record();
  anthy_dic_reload_use_dic();
  anthy_dic_reload_private_dic();

  xs = anthy_cstr_to_xstr(s, ac->encoding);
  retval = anthy_do_context_set_str(ac, xs);
  anthy_free_xstr(xs);
  return retval;
}

/** (API) 文節長の変更 */
void
anthy_resize_segment(struct anthy_context *ac, int nth, int resize)
{
  anthy_dic_activate_session(ac->dic_session);
  anthy_do_resize_segment(ac, nth, resize);
}

/** (API) 変換の状態の取得 */
int
anthy_get_stat(struct anthy_context *ac, struct anthy_conv_stat *s)
{
  s->nr_segment = ac->seg_list.nr_segments;
  return 0;
}

/** (API) 文節の状態の取得 */
int
anthy_get_segment_stat(struct anthy_context *ac, int n,
		       struct anthy_segment_stat *s)
{
  struct seg_ent *seg;
  seg = anthy_get_nth_segment(&ac->seg_list, n);
  if (seg) {
    s->nr_candidate = seg->nr_cands;
    s->seg_len = seg->str.len;
    return 0;
  }
  return -1;
}

/** (API) 文節の取得 */
int
anthy_get_segment(struct anthy_context *ac, int nth_seg,
		  int nth_cand, char *buf, int buflen)
{
  struct seg_ent *seg;
  char *p;
  int len;

  /* 文節を取り出す */
  if (nth_seg < 0 || nth_seg >= ac->seg_list.nr_segments) {
    return -1;
  }
  seg = anthy_get_nth_segment(&ac->seg_list, nth_seg);

  /* 文節から候補を取り出す */
  if ((nth_cand < 0 || nth_cand >= seg->nr_cands) &&
      nth_cand != NTH_UNCONVERTED_CANDIDATE) {
    return -1;
  }
  if (nth_cand == NTH_UNCONVERTED_CANDIDATE) {
    /* 変換前の文字列を取得する */
    p = anthy_xstr_to_cstr(&seg->str, ac->encoding);
  } else {
    p = anthy_xstr_to_cstr(&seg->cands[nth_cand]->str, ac->encoding);
  }

  /* バッファに書き込む */
  len = strlen(p);
  if (!buf) {
    free(p);
    return len;
  }
  if (len + 1 > buflen) {
    /* バッファが足りません */
    free(p);
    return -1;
  }
  strcpy(buf, p);
  free(p);
  return len;
}

/* すべての文節がコミットされたかcheckする */
static int
commit_all_segment_p(struct anthy_context *ac)
{
  int i;
  struct seg_ent *se;
  for (i = 0; i < ac->seg_list.nr_segments; i++) {
    se = anthy_get_nth_segment(&ac->seg_list, i);
    if (se->committed < 0) {
      return 0;
    }
  }
  return 1;
}

/** (API) 文節の確定 */
int
anthy_commit_segment(struct anthy_context *ac, int s, int c)
{
  struct seg_ent *seg;
  if (!ac->str.str) {
    return -1;
  }
  if (s < 0 || s >= ac->seg_list.nr_segments) {
    return -1;
  }
  if (commit_all_segment_p(ac)) {
    /* すでに全てのセグメントがコミットされている */
    return -1;
  }

  anthy_dic_activate_session(ac->dic_session);
  seg = anthy_get_nth_segment(&ac->seg_list, s);
  if (c == NTH_UNCONVERTED_CANDIDATE) {
    /*
     * 変換前の文字列がコミットされたので，それに対応する候補の番号を探す
     */
    int i;
    for (i = 0; i < seg->nr_cands; i++) {
      if (!anthy_xstrcmp(&seg->str, &seg->cands[i]->str)) {
	c = i;
      }
    }
  }
  if (c < 0 || c >= seg->nr_cands) {
    return -1;
  }
  seg->committed = c;

  if (commit_all_segment_p(ac)) {
    /* 今、すべてのセグメントがコミットされた */
    anthy_proc_commit(&ac->seg_list, &ac->split_info);
  }
  return 0;
}

/** (API) 開発用 */
void
anthy_print_context(struct anthy_context *ac)
{
  anthy_do_print_context(ac, default_encoding);
}

/** (API) Anthy ライブラリのバージョンを表す文字列を返す
 * 共有ライブラリでは外部変数のエクスポートは好ましくないので関数にしてある
 */
const char *
anthy_get_version_string (void)
{
#ifdef VERSION
  return VERSION;
#else  /* just in case */
  return "(unknown)";
#endif
}

int
anthy_context_set_encoding(struct anthy_context *ac, int encoding)
{
#ifdef USE_UCS4
  if (!ac) {
    default_encoding = encoding;
  } else {
    ac->encoding = encoding;
  }
  return encoding;
#else
  (void)ac;
  (void)encoding;
  return ANTHY_EUC_JP_ENCODING;
#endif
}

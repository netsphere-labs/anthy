/*
 * 文節に対して候補のリストを生成する。
 * make_candidates()がcontext管理部から呼ばれる。
 *
 * 候補の生成は次の方法で行う
 * (1)splitterが割り当てた品詞に対してproc_splitter_info()
 *    から候補を生成する
 * (2)ひらがなのみとカタカナのみの候補を生成する
 * (3)最後の文字を助詞と解釈して無理矢理候補を生成する
 */
/*
 * Funded by IPA未踏ソフトウェア創造事業 2001 9/30
 * Copyright (C) 2000-2005 TABATA Yusuke
 * Copyright (C) 2004-2005 YOSHIDA Yuichi
 * Copyright (C) 2002 UGAWA Tomoharu
 *
 * $Id: compose.c,v 1.21 2005/05/15 04:15:59 yusuke Exp $
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

#include <anthy.h> /* for ANTHY_*_ENCODING */
#include <conf.h>
#include <dic.h>
#include <splitter.h>
#include <segment.h>
#include "wordborder.h"


/* 郵便番号のindexを作る atoiで上位の桁が落ちているので
 * 0を追加して3桁もしくは7桁のindexを作成する
 */
static void
make_zipcode_index(long long num, char *buf)
{
  const char *fmt = "";
  if (num < 10) {
    fmt = "00%d ";
  } else if (num < 100) {
    fmt = "0%d ";
  } else if (num < 1000) {
    fmt = "%d ";
  } else if (num < 10000) {
    fmt = "000%d ";
  } else if (num < 100000) {
    fmt = "00%d ";
  } else if (num < 1000000) {
    fmt = "0%d ";
  } else {
    fmt = "%d ";
  }
  sprintf(buf, fmt, (int)num);
}

/* 郵便番号辞書から探す */
static xstr *
search_zipcode_dict(xstr *xs, long long num)
{
  FILE *fp;
  char buf[1000];
  char index[30];
  int len;

  if (xs->len != 3 && xs->len != 7) {
    return NULL;
  }

  if (num > 9999999 || num < 1) {
    return NULL;
  }
  fp = fopen(anthy_conf_get_str("ZIPDICT_EUC"), "r");
  if (!fp) {
    return NULL;
  }
  make_zipcode_index(num, index);
  len = strlen(index);
  while (fgets(buf, 1000, fp)) {
    if (!strncmp(buf, index, len)) {
      char *tmp;
      /* 改行を消す */
      buf[strlen(buf) - 1] = 0;
      /* 一番最後のスペースのあとを地名としてとりだす(<=まずい)*/
      tmp = strrchr(buf, ' ');
      tmp ++;
      fclose(fp);
      return anthy_cstr_to_xstr(tmp, ANTHY_EUC_JP_ENCODING);
    }
  }
  fclose(fp);
  return NULL;
}

static struct cand_ent *
alloc_cand_ent(void)
{
  struct cand_ent *ce;
  ce = (struct cand_ent *)malloc(sizeof(struct cand_ent));
  ce->nr_words = 0;
  ce->elm = NULL;
  ce->mw = NULL;
  ce->core_elm_index = -1;
  return ce;
}

/*
 * 候補を複製する
 */
static struct cand_ent *
dup_candidate(struct cand_ent *ce)
{
  struct cand_ent *ce_new;
  int i;
  ce_new = alloc_cand_ent();
  ce_new->nr_words = ce->nr_words;
  ce_new->str.len = ce->str.len;
  ce_new->str.str = anthy_xstr_dup_str(&ce->str);
  ce_new->elm = malloc(sizeof(struct cand_elm)*ce->nr_words);
  ce_new->flag = ce->flag;
  ce_new->core_elm_index = ce->core_elm_index;
  ce_new->mw = ce->mw;
  ce_new->score = ce->score;

  for (i = 0 ; i < ce->nr_words ; i++) {
    ce_new->elm[i] = ce->elm[i];
  }
  return ce_new;
}

/** 文節に候補を追加する */
static void
push_back_candidate(struct seg_ent *seg, struct cand_ent *ce)
{
  /* seg_entに候補ceを追加 */
  seg->nr_cands++;
  seg->cands = (struct cand_ent **)
    realloc(seg->cands, sizeof(struct cand_ent *) * seg->nr_cands);
  seg->cands[seg->nr_cands - 1] = ce;
}


static void
push_back_zipcode_candidate(struct seg_ent *seg)
{
  struct cand_ent *ce;
  long long code;
  xstr *str;

  code = anthy_xstrtoll(&seg->str);
  if (code == -1) {
    return ;
  }
  str = search_zipcode_dict(&seg->str, code);
  if (!str) {
    return ;
  }

  ce = alloc_cand_ent();
  ce->str = *str;
  ce->flag = CEF_SINGLEWORD;
  push_back_candidate(seg, ce);
  free(str);
}

static void
push_back_guessed_candidate(struct seg_ent *seg)
{
  xchar xc;
  xstr *xs;
  struct cand_ent *ce;
  if (seg->str.len < 2) {
    return ;
  }
  /* 最後の文字は助詞か？ */
  xc = seg->str.str[seg->str.len - 1];
  if (!(anthy_get_xchar_type(xc) & XCT_DEP)) {
    return ;
  }
  /* 最後の文字以外をカタカナにしてみる */
  ce = alloc_cand_ent();
  xs = anthy_xstr_hira_to_kata(&seg->str);
  xs->str[xs->len-1] = xc;
  ce->str.str = anthy_xstr_dup_str(xs);
  ce->str.len = xs->len;
  ce->flag = CEF_GUESS;
  anthy_free_xstr(xs);
  push_back_candidate(seg, ce);
}

/** 再帰で1単語ずつ候補を割当てていく */
static int
enum_candidates(struct seg_ent *seg,
		struct cand_ent *ce,
		int from, int n)
{
  int i, p;
  struct cand_ent *cand;
  int nr_cands = 0;

  if (n == ce->mw->nr_parts) {
    /* 完成形 */
    /* 文節後部の解析しなかった部分を候補文字列に追加 */
    xstr tail;
    tail.len = seg->len - from;
    tail.str = &seg->str.str[from];
    anthy_xstrcat(&ce->str, &tail);
    push_back_candidate(seg, dup_candidate(ce));
    return 1;
  }

  p = anthy_get_nr_dic_ents(ce->elm[n].se, &ce->elm[n].str);
  /* 品詞不定の場合には未変換で次の単語へ行く */
  if (anthy_wtype_get_pos(ce->elm[n].wt) == POS_INVAL ||
      anthy_wtype_get_pos(ce->elm[n].wt) == POS_NONE ||
      p == 0) {
    xstr xs;
    xs.len = ce->elm[n].str.len;
    xs.str = &seg->str.str[from];
    cand = dup_candidate(ce);
    cand->elm[n].nth = -1;
    cand->elm[n].id = -1;
    anthy_xstrcat(&cand->str, &xs);
    nr_cands = enum_candidates(seg,cand,
			       from + xs.len,
			       n + 1);
    anthy_release_cand_ent(cand);
    return nr_cands;
  }

  /* 品詞が割当てられているので、その品詞にマッチするものを割当てる */
  for (i = 0; i < p; i++) {
    wtype_t wt;
    anthy_get_nth_dic_ent_wtype(ce->elm[n].se, &ce->str, i, &wt);
    anthy_wtype_set_ct(&ce->elm[n].wt, CT_NONE);
    if (anthy_wtypecmp(ce->elm[n].wt, wt)) {
      xstr word, yomi;
      yomi.len = ce->elm[n].str.len;
      yomi.str = &seg->str.str[from];
      cand = dup_candidate(ce);
      anthy_get_nth_dic_ent_str(cand->elm[n].se,
				&yomi, i, &word);
      cand->elm[n].nth = i;
      cand->elm[n].id = anthy_get_nth_dic_ent_id(ce->elm[n].se, i);

      /* 単語の本体 */
      anthy_xstrcat(&cand->str, &word);
      free(word.str);
      /* 自分を再帰呼び出しして続きをわりあてる */
      nr_cands += enum_candidates(seg, cand, 
				  from + yomi.len,
				  n+1);
      anthy_release_cand_ent(cand);
    }
  }
  return nr_cands;
}

/**
 * 文節全体を含む一単語(単漢字を含む)の候補を生成する
 */
static void
push_back_singleword_candidate(struct seg_ent *seg)
{
  seq_ent_t se;
  struct cand_ent *ce;
  wtype_t wt;
  int i, n;
  xstr xs;

  se = anthy_get_seq_ent_from_xstr(&seg->str);
  n = anthy_get_nr_dic_ents(se, &seg->str);
  /* 辞書の各エントリに対して */
  for (i = 0; i < n; i++) {
    int ct;
    /* 品詞を取り出して */
    anthy_get_nth_dic_ent_wtype(se, &seg->str, i, &wt);
    ct = anthy_wtype_get_ct(wt);
    /* 終止形か活用しないものの原形なら */
    if (ct == CT_SYUSI || ct == CT_NONE) {
      ce = alloc_cand_ent();
      anthy_get_nth_dic_ent_str(se,&seg->str, i, &xs);
      ce->str.str = xs.str;
      ce->str.len = xs.len;
      ce->flag = CEF_SINGLEWORD;
      push_back_candidate(seg, ce);
    }
  }
}

static void
push_back_noconv_candidate(struct seg_ent *seg)
{
  /* 無変換で片仮名になる候補と平仮名のみになる候補を追加 */
  struct cand_ent *ce;
  xstr *xs;

  /* ひらがなのみ */
  ce = alloc_cand_ent();
  ce->str.str = anthy_xstr_dup_str(&seg->str);
  ce->str.len = seg->str.len;
  ce->flag = CEF_HIRAGANA;
  push_back_candidate(seg, ce);

  /* 次にカタカナ */
  ce = alloc_cand_ent();
  xs = anthy_xstr_hira_to_kata(&seg->str);
  ce->str.str = anthy_xstr_dup_str(xs);
  ce->str.len = xs->len;
  ce->flag = CEF_KATAKANA;
  anthy_free_xstr(xs);
  push_back_candidate(seg, ce);
}

static void
make_cand_elem_from_word_list(struct seg_ent *se,
			      struct cand_ent *ce,
			      struct word_list *wl,
			      int index)
{
  int i; 
  int from = wl->from - se->from;
  for (i = 0; i < NR_PARTS; ++i) {
    struct part_info *part = &wl->part[i];
    xstr core_xs;
    if (part->len == 0) {
      /* 長さの無いpartは無視する */
      continue;
    }
    if (i == PART_CORE) {
      ce->core_elm_index = i + index;
    }
    core_xs.str = &se->str.str[from];
    core_xs.len = part->len;
    ce->elm[i + index].se = anthy_get_seq_ent_from_xstr(&core_xs);
    ce->elm[i + index].str.str = core_xs.str;
    ce->elm[i + index].str.len = core_xs.len;
    ce->elm[i + index].wt = part->wt;
    ce->elm[i + index].ratio = part->ratio * (wl->len - wl->weak_len);
    from += part->len;
  }
}


/** まずwordlistを持つmetawordからmeta_wordを取り出す */
static void
make_candidate_from_simple_metaword(struct seg_ent *se,
				    struct meta_word *mw,
				    struct meta_word *top_mw)
{
  /*
   * 各単語の品詞が決定された状態でコミットされる。
   */
  struct cand_ent *ce;
  struct word_list *wl = mw->wl;

  /* 複数(1も含む)の単語で構成される文節に単語を割当てていく */
  ce = alloc_cand_ent();
  ce->nr_words = mw->nr_parts;
  ce->str.str = NULL;
  ce->str.len = 0;
  ce->elm = calloc(sizeof(struct cand_elm),ce->nr_words);
  ce->mw = top_mw;
  ce->score = 0;

  /* 接頭辞, 自立語部, 接尾辞, 付属語 */
  make_cand_elem_from_word_list(se, ce, wl, 0);

  ce->flag = (se->best_mw == mw) ? CEF_BEST : CEF_NONE;
  enum_candidates(se, ce, 0, 0);
  anthy_release_cand_ent(ce);
}

/** combinedなmetawordは二つの語を合体して一つの語として出す */
static void
make_candidate_from_combined_metaword(struct seg_ent *se,
				      struct meta_word *mw,
				      struct meta_word *top_mw)
{
  /*
   * 各単語の品詞が決定された状態でコミットされる。
   */
  struct cand_ent *ce;
  int score;

  /* 複数(1も含む)の単語で構成される文節に単語を割当てていく */
  ce = alloc_cand_ent();
  ce->nr_words = mw->nr_parts;
  ce->str.str = NULL;
  ce->str.len = 0;
  ce->elm = calloc(sizeof(struct cand_elm),ce->nr_words);
  ce->mw = top_mw;

  /* 接頭辞, 自立語部, 接尾辞, 付属語 */
  make_cand_elem_from_word_list(se, ce, mw->mw1->wl, 0);
  if (mw->mw2) {
    make_cand_elem_from_word_list(se, ce, mw->mw2->mw1->wl, NR_PARTS);
  }
  /* 構造を評価する */
  score = mw->score;

  ce->score = score;
  ce->flag = (se->best_mw == mw) ? CEF_BEST : CEF_NONE;
  enum_candidates(se, ce, 0, 0);
  anthy_release_cand_ent(ce);
}


/** splitterの情報を利用して候補を生成する
 */
static void
proc_splitter_info(struct seg_ent *se,
		   struct meta_word *mw,
		   struct meta_word *top_mw)
{
  enum mw_status st;
  if (!mw) return;

  /* まずwordlistを持つmetawordの場合 */
  if (mw->wl && mw->wl->len) {
    make_candidate_from_simple_metaword(se, mw, top_mw);
    return;
  }
  
  st = anthy_metaword_type_tab[mw->type].status;
  switch (st) {
  case MW_STATUS_WRAPPED:
    /* wrapされたものの情報を取り出す */
    proc_splitter_info(se, mw->mw1, top_mw);
    break;
  case MW_STATUS_COMBINED:
    make_candidate_from_combined_metaword(se, mw, top_mw);
    break;
  case MW_STATUS_COMPOUND:
    /* 連文節の葉 */
    {
      struct cand_ent *ce;
      ce = alloc_cand_ent();
      ce->str.str = anthy_xstr_dup_str(&mw->cand_hint);
      ce->str.len = mw->cand_hint.len;
      ce->flag = CEF_COMPOUND;
      ce->mw = top_mw;
      push_back_candidate(se, ce);
    }
    break;
  case MW_STATUS_COMPOUND_PART:
    /* 連文節の個々の文節を結合して一つの文節としてみたもの */
    /* BREAK THROUGH */
  case MW_STATUS_OCHAIRE:
    {
    /* metawordを持たない */
      /* 候補文字列がダイレクトに指定された */
      {
	struct cand_ent *ce;
	ce = alloc_cand_ent();
	ce->str.str = anthy_xstr_dup_str(&mw->cand_hint);
	ce->str.len = mw->cand_hint.len;
	ce->mw = top_mw;
	ce->flag = (st == MW_STATUS_OCHAIRE) ? CEF_OCHAIRE : CEF_COMPOUND_PART;

	if (mw->len < se->len) {
	  /* metawordでカバーされてない領域の文字列を付ける */
	  xstr xs;
	  xs.str = &se->str.str[mw->len];
	  xs.len = se->len - mw->len;
	  anthy_xstrcat(&ce->str ,&xs);
	}
	push_back_candidate(se, ce);
      }
      break;
    }
  case MW_STATUS_NONE:
    break;
  default:
    break;
  }
}

/** context.cから呼出されるもっとも大物
 * 一つ以上の候補を必ず生成する
 */
void
anthy_do_make_candidates(struct seg_ent *se)
{
  int i, limit = 0;

  /* 0番目のmetawordのスコアの1/3を候補生成の下限にする */
  if (se->nr_metaword) {
    int best = se->mw_array[0]->score;
    if (best > RATIO_BASE) {
      best = RATIO_BASE;
    }
    limit = best / 3;
  }

  /* hmmでのbestのmeta_wordから候補を生成*/
  proc_splitter_info(se, se->best_mw, se->best_mw);
  /* limitよりも高いscoreを持つmetawordから候補を生成する */
  for (i = 0; i < se->nr_metaword; i++) {
    struct meta_word *mw = se->mw_array[i];
    if (mw->score > limit && mw != se->best_mw) {
      /**/
      proc_splitter_info(se, mw, mw);
    }
  }
  /* 単漢字などの候補 */
  push_back_singleword_candidate(se);
  /* 郵便番号 */
  push_back_zipcode_candidate(se);
  /* ひらがな、カタカナの無変換エントリを作る */
  push_back_noconv_candidate(se);

  /* 候補が二つしか無いときは最後が助詞にで残りが平仮名の候補を作れるか試す */
  push_back_guessed_candidate(se);
}


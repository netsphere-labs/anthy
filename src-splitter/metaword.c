/*
 * 文節もしくは単語を一つ以上セットにしてmetawordとして扱う。
 * ここでは各種のmetawordを生成する
 *
 * init_metaword_tab() metaword処理のための情報を構成する
 * anthy_make_metaword_all() context中のmetawordを構成する
 * anthy_print_metaword() 指定されたmetawordを表示する
 *
 * Funded by IPA未踏ソフトウェア創造事業 2001 10/29
 * Copyright (C) 2000-2004 TABATA Yusuke
 * Copyright (C) 2004 YOSHIDA Yuichi
 * Copyright (C) 2000-2003 UGAWA Tomoharu
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <record.h>
#include <splitter.h>
#include <xstr.h>
#include <segment.h>
#include <segclass.h>
#include "wordborder.h"

/* 各種meta_wordをどのように処理するか */
struct metaword_type_tab_ anthy_metaword_type_tab[] = {
  {MW_DUMMY,0,MW_STATUS_NONE,MW_CHECK_WL_STR},
  {MW_SINGLE,0,MW_STATUS_NONE,MW_CHECK_WL_SINGLE},
  {MW_WRAP,0,MW_STATUS_WRAPPED,MW_CHECK_WL_WRAP},
  {MW_COMPOUND_HEAD,0,MW_STATUS_NONE,MW_CHECK_COMPOUND},
  {MW_COMPOUND,0,MW_STATUS_NONE,MW_CHECK_NONE},
  {MW_COMPOUND_LEAF,0,MW_STATUS_COMPOUND,MW_CHECK_NONE},
  {MW_COMPOUND_PART,0,MW_STATUS_COMPOUND_PART,MW_CHECK_WL_STR},
  {MW_NAMEPAIR,0,MW_STATUS_COMBINED,MW_CHECK_PAIR},
  {MW_V_RENYOU_A,100,MW_STATUS_COMBINED,MW_CHECK_BORDER},
  {MW_V_RENYOU_NOUN,100,MW_STATUS_COMBINED,MW_CHECK_BORDER},
  {MW_NUM_XX,0,MW_STATUS_COMBINED,MW_CHECK_PAIR},
  {MW_NOUN_NOUN_PREFIX,0,MW_STATUS_COMBINED,MW_CHECK_PAIR},
  {MW_OCHAIRE,0,MW_STATUS_OCHAIRE,MW_CHECK_OCHAIRE},
  /**/
  {MW_SENTENCE,0,MW_STATUS_NONE,MW_CHECK_PAIR},
  {MW_MODIFIED,0,MW_STATUS_NONE,MW_CHECK_PAIR},
  {MW_END,0,MW_STATUS_NONE,MW_CHECK_NONE}
};


/* コンテキスト中にmetawordを追加する */
void
anthy_commit_meta_word(struct splitter_context *sc,
		       struct meta_word *mw)
{
  struct word_split_info_cache *info = sc->word_split_info;
  mw->score += anthy_metaword_type_tab[mw->type].score;
  /* 同じ開始点を持つノードのリスト */
  mw->next = info->cnode[mw->from].mw;
  info->cnode[mw->from].mw = mw;
}

static void
anthy_do_print_metaword(struct splitter_context *sc,
			struct meta_word *mw,
			int indent)
{
  int i;
  for (i = 0; i < indent; i++) {
    printf(" ");
  }
  printf("*meta word type=%d(%d-%d)%d:score=%d:seg_class=%d*\n",
	 mw->type, mw->from, mw->len, mw->mw_count, mw->score, mw->seg_class);
  if (mw->wl) {
    anthy_print_word_list(sc, mw->wl);
  }
  if (mw->mw1) {
    anthy_do_print_metaword(sc, mw->mw1, indent + 1);
  }    
  if (mw->mw2) {
    anthy_do_print_metaword(sc, mw->mw2, indent + 1);
  }
}

void
anthy_print_metaword(struct splitter_context *sc,
		     struct meta_word *mw)
{
  anthy_do_print_metaword(sc, mw, 0);
}

static struct meta_word *
alloc_metaword(struct splitter_context *sc)
{
  struct meta_word *mw;
  mw = anthy_smalloc(sc->word_split_info->MwAllocator);
  mw->weak_len = 0;
  mw->type = MW_SINGLE;
  mw->mw_count = 1;
  mw->score = 0;
  mw->wl = NULL;
  mw->mw1 = NULL;
  mw->mw2 = NULL;
  mw->parent = NULL;
  mw->cand_hint.str = NULL;
  mw->cand_hint.len = 0;
  mw->seg_class = SEG_HEAD;
  mw->can_use = ok;
  return mw;
}

static void
get_surrounding_text(struct splitter_context* sc,
		     struct word_list* wl,
		     xstr* xs_pre, xstr* xs_post)
{
    int post_len = wl->part[PART_DEPWORD].len + wl->part[PART_POSTFIX].len;
    int pre_len = wl->part[PART_PREFIX].len;

    xs_pre->str = sc->ce[wl->from].c;
    xs_pre->len = pre_len;
    xs_post->str = sc->ce[wl->from + wl->len - post_len].c;
    xs_post->len = post_len;
}

static struct meta_word*
make_compound_nth_metaword(struct splitter_context* sc, 
			   compound_ent_t ce, int nth,
			   struct word_list* wl,
			   enum metaword_type type)
{
  int i;
  int len = 0;
  int from = wl->from;
  int seg_num = anthy_compound_get_nr_segments(ce);
  struct meta_word* mw;
  xstr xs_pre, xs_core, xs_post;

  get_surrounding_text(sc, wl, &xs_pre, &xs_post);

  for (i = 0; i <= nth; ++i) {
    from += len;
    len = anthy_compound_get_nth_segment_len(ce, i);
    if (i == 0) {
      len += xs_pre.len;
    }
    if (i == seg_num - 1) {
      len += xs_post.len;
    }
  }
  
  mw = alloc_metaword(sc);
  mw->from = from;
  mw->len = len;
  mw->type = type;
  mw->score = wl->score;
  mw->seg_class = wl->seg_class;

  anthy_compound_get_nth_segment_xstr(ce, nth, &xs_core);
  if (nth == 0) {
    anthy_xstrcat(&mw->cand_hint, &xs_pre);
  }
  anthy_xstrcat(&mw->cand_hint, &xs_core);
  if (nth == seg_num - 1) {
    anthy_xstrcat(&mw->cand_hint, &xs_post);
  }
  return mw;
}

/*
 * 複合語用のmeta_wordを作成する。
 */
static void
make_compound_metaword(struct splitter_context* sc, struct word_list* wl)
{
  int i, j;
  seq_ent_t se = wl->part[PART_CORE].seq;
  int ent_num = anthy_get_nr_compound_ents(se);

  for (i = 0; i < ent_num; ++i) {
    compound_ent_t ce = anthy_get_nth_compound_ent(se, i);
    int seg_num = anthy_compound_get_nr_segments(ce);
    struct meta_word *mw = NULL;
    struct meta_word *mw2 = NULL;

    for (j = seg_num - 1; j >= 0; --j) {
      enum metaword_type type;
      mw = make_compound_nth_metaword(sc, ce, j, wl, MW_COMPOUND_LEAF);
      anthy_commit_meta_word(sc, mw);

      type = j == 0 ? MW_COMPOUND_HEAD : MW_COMPOUND;
      mw2 = anthy_do_combine_metaword(sc, type, mw, mw2);
    }
  }
}

/*
 * 複合語の中の個々の文節を結合したmeta_wordを作成する。
 */
static void
make_compound_part_metaword(struct splitter_context* sc, struct word_list* wl)
{
  int i, j, k;
  seq_ent_t se = wl->part[PART_CORE].seq;
  int ent_num = anthy_get_nr_compound_ents(se);

  for (i = 0; i < ent_num; ++i) {
    compound_ent_t ce = anthy_get_nth_compound_ent(se, i);
    int seg_num = anthy_compound_get_nr_segments(ce);
    struct meta_word *mw = NULL;
    struct meta_word *mw2 = NULL;

    /* 後ろから */
    for (j = seg_num - 1; j >= 0; --j) {
      mw = make_compound_nth_metaword(sc, ce, j, wl, MW_COMPOUND_PART);
      for (k = j - 1; k >= 0; --k) {
	mw2 = make_compound_nth_metaword(sc, ce, k, wl, MW_COMPOUND_PART);
	mw2->len += mw->len;
	anthy_xstrcat(&mw2->cand_hint, &mw->cand_hint);

	anthy_commit_meta_word(sc, mw2);	
	mw = mw2;
      }
    } 
  }
}

/*
 * 単文節単語
 */
static void
make_simple_metaword(struct splitter_context *sc, struct word_list* wl)
{
  struct meta_word *mw = alloc_metaword(sc);
  mw->wl = wl;
  mw->from = wl->from;
  mw->len = wl->len;
  mw->weak_len = wl->weak_len;
  mw->score = wl->score;
  mw->type = MW_SINGLE;
  mw->seg_class = wl->seg_class;
  mw->nr_parts = NR_PARTS;
  anthy_commit_meta_word(sc, mw);
}

/*
 * wordlist一個からなる、metawordを作成
 */
static void
make_metaword_from_word_list(struct splitter_context *sc)
{
  int i;
  for (i = 0; i < sc->char_count; i++) {
    struct word_list *wl;
    for (wl = sc->word_split_info->cnode[i].wl;
	 wl; wl = wl->next) {
      if (wl->is_compound) {
	make_compound_part_metaword(sc, wl);
	make_compound_metaword(sc, wl);
      } else {
	make_simple_metaword(sc, wl);
      }
    }
  }
}

/*
 * metawordを実際に結合する
 */
struct meta_word *
anthy_do_combine_metaword(struct splitter_context *sc,
			  enum metaword_type type,
			  struct meta_word *mw, struct meta_word *mw2)
{
  struct meta_word *n;
 
  n = alloc_metaword(sc);
  n->from = mw->from;
  n->len = mw->len + (mw2 ? mw2->len : 0);
  n->weak_len = mw->weak_len + (mw2 ? mw2->weak_len : 0);
  if (mw2) {
    n->score = sqrt(mw->score) * sqrt(mw2->score);    
  } else {
    n->score = mw->score;    
  }
  n->type = type;
  n->mw1 = mw;
  n->mw2 = mw2;
  n->seg_class = mw2 ? mw2->seg_class : mw->seg_class;
  n->nr_parts = mw->nr_parts + (mw2 ? mw2->nr_parts : 0);
  anthy_commit_meta_word(sc, n);
  return n;
}

/*
 * 動詞連用形 + 形容詞化接尾語 「〜しやすい」など
 */
static void
try_combine_v_renyou_a(struct splitter_context *sc,
		       struct meta_word *mw, struct meta_word *mw2)
{
  wtype_t w2 = mw2->wl->part[PART_CORE].wt;

  if (mw->wl->head_pos == POS_V &&
      mw->wl->tail_ct == CT_RENYOU &&
      anthy_wtype_get_pos(w2) == POS_A) {
    /* 形容詞ではあるので次のチェック */
    if (anthy_get_seq_ent_wtype_freq(mw2->wl->part[PART_CORE].seq, 
				     anthy_wtype_a_tail_of_v_renyou)) {
      anthy_do_combine_metaword(sc, MW_V_RENYOU_A, mw, mw2);
    }
  }
}

/*
 * 動詞連用形 + 名詞化接尾語(#D2T35) 「入れ たて(のお茶)」など
 */
static void
try_combine_v_renyou_noun(struct splitter_context *sc,
			  struct meta_word *mw, struct meta_word *mw2)
{
  wtype_t w2 = mw2->wl->part[PART_CORE].wt;
  if (mw->wl->head_pos == POS_V &&
      mw->wl->tail_ct == CT_RENYOU &&
      anthy_wtype_get_pos(w2) == POS_NOUN &&
      anthy_wtype_get_scos(w2) == SCOS_T40) {
    anthy_do_combine_metaword(sc, MW_V_RENYOU_NOUN, mw, mw2);
  }
}


/*
 * 名詞 + 名詞接尾語(#N2T35, #N2T30) 「サーバ 用」など
 */
static void
try_combine_noun_noun_postfix(struct splitter_context *sc,
			      struct meta_word *mw, struct meta_word *mw2)
{
  wtype_t w1 = mw->wl->part[PART_CORE].wt;
  if (anthy_wtype_get_pos(w1) == POS_NOUN &&
      mw->wl->part[PART_CORE].len > 1 &&
      mw->wl->part[PART_POSTFIX].len == 0 &&
      mw->wl->part[PART_DEPWORD].len == 0 &&
      mw2->wl->part[PART_CORE].len > 1 &&
      anthy_wtype_get_pos(mw2->wl->part[PART_CORE].wt) == POS_N2T &&
      anthy_get_seq_ent_wtype_freq(mw2->wl->part[PART_CORE].seq, 
				   anthy_wtype_noun_and_postfix)) {
    anthy_do_combine_metaword(sc, MW_NOUN_NOUN_PREFIX, mw, mw2);
  }
}

/*
 * 氏 + 名を結合する
 */
static void
try_combine_name(struct splitter_context *sc,
		 struct meta_word *mw, struct meta_word *mw2)
{
  int f, f2;
  f = anthy_get_seq_flag(mw->wl->part[PART_CORE].seq);
  f2 = anthy_get_seq_flag(mw2->wl->part[PART_CORE].seq);

  if ((f & NF_FAMNAME) && (f2 & NF_FSTNAME)) {
    if (anthy_wtype_get_scos(mw->wl->part[PART_CORE].wt) == SCOS_FAMNAME &&
	anthy_wtype_get_scos(mw2->wl->part[PART_CORE].wt) == SCOS_FSTNAME) {
      anthy_do_combine_metaword(sc, MW_NAMEPAIR, mw, mw2);
    }
  }
}

static void
try_combine_10_1(struct splitter_context *sc,
		 struct meta_word *mw, struct meta_word *mw2)
{
  int pos1, pos2;
  pos1 = anthy_wtype_get_pos(mw->wl->part[PART_CORE].wt);
  pos2 = anthy_wtype_get_pos(mw2->wl->part[PART_CORE].wt);
  if (pos1 != POS_NUMBER) {
    return ;
  }
  if (pos2 != POS_NUMBER) {
    return ;
  }
  if (anthy_get_seq_ent_wtype_freq(mw->wl->part[PART_CORE].seq,
				   anthy_wtype_n10) &&
      anthy_get_seq_ent_wtype_freq(mw2->wl->part[PART_CORE].seq,
				   anthy_wtype_n1)) {
    anthy_do_combine_metaword(sc, MW_NUM_XX, mw, mw2);
  }
}

/* 右隣のmetawordと結合できるかチェック */
static void
try_combine_metaword(struct splitter_context *sc,
		     struct meta_word *mw1, struct meta_word *mw2)
{
  /**/
  if (!mw1->wl || !mw2->wl) {
    return ;
  }
  /* metawordの結合を行うためには、後続の
     metawordに接頭辞がないことが必要 */
  if (mw2->wl->part[PART_PREFIX].len == 0) {
    try_combine_name(sc, mw1, mw2);
    try_combine_v_renyou_a(sc, mw1, mw2);
    try_combine_v_renyou_noun(sc, mw1, mw2);
    try_combine_noun_noun_postfix(sc, mw1, mw2);
    try_combine_10_1(sc, mw1, mw2);
  }
}


static void
combine_metaword(struct splitter_context *sc)
{
  int i;

  struct word_split_info_cache *info = sc->word_split_info;
  /* metawordの左端によるループ */
  for (i = 0; i < sc->char_count; i++){
    struct meta_word *mw, *mw2;
    /* 各metawordのループ */
    for (mw = info->cnode[i].mw;
	 mw; mw = mw->next) {
      /* metawordが右端に到達していなければ */
      if (mw->len + i < sc->char_count) {
	/* そのmetawordの右metawordのやつひとつと */
	for (mw2 = info->cnode[mw->len+i].mw; 
	     mw2; mw2 = mw2->next) {
	  /* 付属語だけの文節とは結合しない */
	  if (!anthy_seg_class_is_depword(mw2->seg_class)) {
	    /* 結合できるかチェック */
	    try_combine_metaword(sc, mw, mw2);
	  }
	}
      }
    }
  }
}

static void
make_dummy_metaword(struct splitter_context *sc, int from,
		    int len, int orig_len)
{
  int score = 0;
  struct meta_word *mw, *n;

  for (mw = sc->word_split_info->cnode[from].mw; mw; mw = mw->next) {
   if (mw->len != orig_len) continue;
    if (mw->score > score) {
      score = mw->score;
    }
  }

  n = alloc_metaword(sc);
  n->type = MW_DUMMY;
  n->from = from;
  n->len = len;
  n->score = 3 * score * len / orig_len;
  if (mw) {
    mw->nr_parts = 0;
  }
  anthy_commit_meta_word(sc, n);
}

/*
 * 文節を伸ばしたらそれを覚えておく
 */
static void
make_expanded_metaword_all(struct splitter_context *sc)
{
  int i, j;
  if (anthy_select_section("EXPANDPAIR", 0) == -1) {
    return ;
  }
  for (i = 0; i < sc->char_count; i++) {
    for (j = 1; j < sc->char_count - i; j++) {
      /* 全ての部分文字列に対して */
      xstr xs;
      xs.len = j;
      xs.str = sc->ce[i].c;
      if (anthy_select_column(&xs, 0) == 0) {
	/* この部分文字列は過去に拡大の対象となった */
        int k;
        int nr = anthy_get_nr_values();
        for (k = 0; k < nr; k++) {
          xstr *exs;
          exs = anthy_get_nth_xstr(k);
          if (exs && exs->len <= sc->char_count - i) {
            xstr txs;
            txs.str = sc->ce[i].c;
            txs.len = exs->len;
            if (!anthy_xstrcmp(&txs, exs)) {
              make_dummy_metaword(sc, i, txs.len, j);
            }
          }
        }
      }
    }
  }
}

/* お茶入れ学習のmetawordを作る */
static void
make_ochaire_metaword(struct splitter_context *sc,
		      int from, int len)
{
  struct meta_word *mw;
  int count;
  int s;
  int j;
  int seg_len;
  int mw_len = 0;
  xstr* xs;

  (void)len;

  /* 文節数を取得 */
  count = anthy_get_nth_value(0);
  /* 一番右の文節をのぞいた文字数の合計を計算 */
  for (s = 0, j = 0; j < count - 1; j++) {
    s += anthy_get_nth_value(j * 2 + 1);
  }
  /* 一番右の文節のmetawordを構成 */
  seg_len = anthy_get_nth_value((count - 1) * 2 + 1);
  mw = alloc_metaword(sc);
  mw->type = MW_OCHAIRE;
  mw->from = from + s;
  mw->len = seg_len;
  xs = anthy_get_nth_xstr((count - 1) * 2 + 2);
  mw->cand_hint.str = xs->str;
  mw->cand_hint.len = xs->len;
  anthy_commit_meta_word(sc, mw);
  mw_len += seg_len;
  /* それ以外の文節でmetawordを構成 */
  for (j-- ; j >= 0; j--) {
    struct meta_word *n;
    seg_len = anthy_get_nth_value(j * 2 + 1);
    s -= seg_len;
    n = alloc_metaword(sc);
    n->type = MW_OCHAIRE;
    /* 右のmetawordをつなぐ */
    n->mw1 = mw;
    n->from = from + s;
    n->len = seg_len;
    xs = anthy_get_nth_xstr(j * 2 + 2);
    n->cand_hint.str = xs->str;
    n->cand_hint.len = xs->len;
    anthy_commit_meta_word(sc, n);
    mw = n;
    mw_len += seg_len;
  } 
}

/*
 * 複数の文節の組を履歴から検索する
 */
static void
make_ochaire_metaword_all(struct splitter_context *sc)
{
  int i;
  if (anthy_select_section("OCHAIRE", 0) == -1) {
    return ;
  }

  for (i = 0; i < sc->char_count; i++) {
    xstr xs;
    xs.len = sc->char_count - i;
    xs.str = sc->ce[i].c;
    if (anthy_select_longest_column(&xs) == 0) {
      xstr* key;
      int len;
      anthy_mark_column_used();
      key = anthy_get_index_xstr();
      len = key->len;

      make_ochaire_metaword(sc, i, len);
      /* 今回見つかった meta_word の次の文字から始める */
      i += len - 1;
      break;
    }
  }
}

/*
 * metawordの後ろの雑多な文字をくっつけたmetawordを構成する
 */
static void
make_metaword_with_depchar(struct splitter_context *sc,
			   struct meta_word *mw)
{
  int j;
  int destroy_seg_class = 0;

  /* metawordの直後の文字の種類を調べる */
  int type = anthy_get_xchar_type(*sc->ce[mw->from + mw->len].c);
  if (!(type & XCT_SYMBOL) &&
      !(type & XCT_PART)) {
    return;
  }

  /* 同じ種類の文字でなければくっつけるのをうちきり */
  for (j = 0; mw->from + mw->len + j < sc->char_count; j++) {
    int p = mw->from + mw->len + j;
    if ((anthy_get_xchar_type(*sc->ce[p].c) != type)) {
      break;
    }
    if (*sc->ce[p].c != *sc->ce[p - 1].c) {
      destroy_seg_class = 1;
    }
  }

  /* 上のループを抜けた時、jには独立できない文字の数が入っている */

  /* 独立できない文字があるので、それを付けたmetawordを作る */
  if (j > 0) {
    struct meta_word *n;
    n = alloc_metaword(sc);
    n->type = MW_WRAP;
    n->mw1 = mw;
    n->from = mw->from;
    n->len = mw->len + j;
    n->weak_len = mw->weak_len + j;
    n->score = mw->score;
    n->nr_parts = mw->nr_parts;
    if (destroy_seg_class) {
      n->seg_class = SEG_DOKURITSUGO;
      n->score /= 10;
    } else {
      n->seg_class = mw->seg_class;
    }
    anthy_commit_meta_word(sc, n);
  }
}

static void 
make_metaword_with_depchar_all(struct splitter_context *sc)
{
  int i;
  struct word_split_info_cache *info = sc->word_split_info;

  /* 全metawordに対して */
  for (i = 0; i < sc->char_count; i++) {
    struct meta_word *mw;
    for (mw = info->cnode[i].mw;
	 mw; mw = mw->next) {
      make_metaword_with_depchar(sc, mw);
    }
  }
}

static int
is_single(xstr* xs)
{
  int i;
  int xct;
  for (i = xs->len - 1; i >= 1; --i) {
    xct = anthy_get_xchar_type(xs->str[i]);
    if (!(xct & XCT_PART)) {
      return 0;
    }
  }
  return 1;
}

static void 
bias_to_single_char_metaword(struct splitter_context *sc)
{
  int i;

  for (i = sc->char_count - 1; i >= 0; --i) {
    struct meta_word *mw;
    xstr xs;
    int xct;

    struct char_node *cnode = &sc->word_split_info->cnode[i];

    /* カッコの場合は一文字で文節を構成できる */
    xct = anthy_get_xchar_type(*sc->ce[i].c);
    if (xct & (XCT_OPEN|XCT_CLOSE)) {
      continue;
    }

    xs.str = sc->ce[i].c;
    for (mw = cnode->mw; mw; mw = mw->next) {
      /* 付属語のみの文節は減点しない */
      if (anthy_seg_class_is_depword(mw->seg_class)) {
	continue;
      } 
      /* 一文字(+直前につながる文字の繰り返し)のスコアを下げる */
      xs.len = mw->len;
      if (is_single(&xs)) {
	mw->score /= 100;
      }
    }
  }
}

void
anthy_make_metaword_all(struct splitter_context *sc)
{
  /* まず、word_listいっこのmetawordを作る */
  make_metaword_from_word_list(sc);

  /* metawordを結合する */
  combine_metaword(sc);

  /* 拡大された文節を処理する */
  make_expanded_metaword_all(sc);

  /* 濁点や長音などの記号、その他の記号を処理 */
  make_metaword_with_depchar_all(sc);

  /* おちゃをいれる */
  make_ochaire_metaword_all(sc);

  /* 一文字の文節は減点 */
  bias_to_single_char_metaword(sc);
}

/* 
 * 指定された領域をカバーするmetawordを数える 
 */
int
anthy_get_nr_metaword(struct splitter_context *sc,
		     int from, int len)
{
  struct meta_word *mw;
  int n;

  for (n = 0, mw = sc->word_split_info->cnode[from].mw;
       mw; mw = mw->next) {
    if (mw->len == len && mw->can_use == ok) {
      n++;
    }
  }
  return n;
}

struct meta_word *
anthy_get_nth_metaword(struct splitter_context *sc,
		      int from, int len, int nth)
{
  struct meta_word *mw;
  int n;
  for (n = 0, mw = sc->word_split_info->cnode[from].mw;
       mw; mw = mw->next) {
    if (mw->len == len && mw->can_use == ok) {
      if (n == nth) {
	return mw;
      }
      n++;
    }
  }
  return NULL;
}

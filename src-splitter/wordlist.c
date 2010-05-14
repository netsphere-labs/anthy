/*
 * 文節の最小単位であるwordlistを構成する
 *
 * init_word_seq_tab()
 *   付属語テーブル中のノードへのポインタの初期化
 * release_word_seq_tab()
 *   付属語テーブルの解放
 * anthy_make_word_list_all() 
 * 文節の形式を満たす部分文字列を列挙する
 *  いくかの経路で列挙されたword_listは
 *  anthy_commit_word_listでsplitter_contextに追加される
 *
 * Funded by IPA未踏ソフトウェア創造事業 2002 2/27
 * Copyright (C) 2000-2004 TABATA Yusuke
 * Copyright (C) 2004 YOSHIDA Yuichi
 * Copyright (C) 2000-2003 TABATA Yusuke, UGAWA Tomoharu
 *
 * $Id: wordlist.c,v 1.50 2002/11/17 14:45:47 yusuke Exp $
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <alloc.h>
#include <record.h>
#include <xstr.h>
#include <wtype.h>
#include <conf.h>
#include <ruleparser.h>
#include <dic.h>
#include <splitter.h>
#include "wordborder.h"

int anthy_score_per_freq = 55;
int anthy_score_per_depword = 950;
int anthy_score_per_len = 2;

static allocator wordseq_rule_ator;

/** 自立語の品詞とその後に続く付属語のグラフ中のノードの対応 */
struct wordseq_rule {
  wtype_t wt; /* 自立語の品詞 */
  int ratio; /* 候補のスコアに対する倍率 */
  const char *wt_name; /* 品詞名(デバッグ用) */
  int node_id; /* この自立語の後ろに続く付属語グラフ中のノードのid */
  struct wordseq_rule *next;
};

/* 単語接続ルール */
static struct wordseq_rule *gRules;

/* デバッグ用 */
void
anthy_print_word_list(struct splitter_context *sc,
		      struct word_list *wl)
{
  xstr xs;
  const char *wn = "---";
  if (!wl) {
    printf("--\n");
    return ;
  }
  /* 接頭辞 */
  xs.len = wl->part[PART_CORE].from - wl->from;
  xs.str = sc->ce[wl->from].c;
  anthy_putxstr(&xs);
  printf(".");
  /* 自立語 */
  xs.len = wl->part[PART_CORE].len;
  xs.str = sc->ce[wl->part[PART_CORE].from].c;
  anthy_putxstr(&xs);
  printf(".");
  /* 接尾辞 */
  xs.len = wl->part[PART_POSTFIX].len;
  xs.str = sc->ce[wl->part[PART_CORE].from + wl->part[PART_CORE].len].c;
  anthy_putxstr(&xs);
  printf("-");
  /* 付属語 */
  xs.len = wl->part[PART_DEPWORD].len;
  xs.str = sc->ce[wl->part[PART_CORE].from +
		  wl->part[PART_CORE].len +
		  wl->part[PART_POSTFIX].len].c;
  anthy_putxstr(&xs);
  if (wl->core_wt_name) {
    wn = wl->core_wt_name;
  }
  printf(" %s %d %d %d\n", wn, wl->score, wl->part[PART_DEPWORD].ratio, wl->seg_class);
}

/** word_listを評価する */
static void
eval_word_list(struct word_list *wl)
{
  struct part_info *part = wl->part;

  /* 自立語のスコアと頻度による加点 */
  wl->score += part[PART_CORE].freq * anthy_score_per_freq;

  /* 付属語に対する加点 */
  if (part[PART_DEPWORD].len) {
    int score;
    int len = part[PART_DEPWORD].len - wl->weak_len;
    if (len > 5) {
      len = 5;
    }
    score = len * part[PART_DEPWORD].ratio * anthy_score_per_depword;
    score /= RATIO_BASE;
    wl->score += score;
  }

  /* 各パートによる調整 */
  wl->score *= part[PART_CORE].ratio;
  wl->score /= RATIO_BASE;
  wl->score *= part[PART_POSTFIX].ratio;
  wl->score /= RATIO_BASE;
  wl->score *= part[PART_PREFIX].ratio;
  wl->score /= RATIO_BASE;
  wl->score *= part[PART_DEPWORD].ratio;
  wl->score /= RATIO_BASE;

  /* 長さによる加点 */
  wl->score += (wl->len - wl->weak_len) * anthy_score_per_len;
}

/** word_listを比較する、枝刈りのためなので、
    厳密な比較である必要は無い */
static int
word_list_same(struct word_list *wl1, struct word_list *wl2)
{
  if (wl1->node_id != wl2->node_id ||
      wl1->score != wl2->score ||
      wl1->from != wl2->from ||
      wl1->len != wl2->len ||
      anthy_wtype_get_pos(wl1->part[PART_CORE].wt) != anthy_wtype_get_pos(wl2->part[PART_CORE].wt) ||
      wl1->head_pos != wl2->head_pos) {
    return 0;
  }
  if (wl1->part[PART_DEPWORD].dc != wl2->part[PART_DEPWORD].dc) {
    return 0;
   }
   return 1;
 }


/** 作ったword_listのスコアを計算してからコミットする */
void 
anthy_commit_word_list(struct splitter_context *sc,
		       struct word_list *wl)
{
  struct word_list *tmp;

  /* 付属語だけのword_listで、長さ0のもやってくるので */
  if (wl->len == 0) return;
  /**/
  wl->last_part = PART_DEPWORD;

  /* 点数計算 */
  eval_word_list(wl);
  /* hmmで使用するクラスの設定 */
  anthy_set_seg_class(wl);

  /* 同じ内容のword_listがないかを調べる */
  for (tmp = sc->word_split_info->cnode[wl->from].wl; tmp; tmp = tmp->next) {
    if (word_list_same(tmp, wl)) {
      return ;
    }
  }
  /* wordlistのリストに追加 */
  wl->next = sc->word_split_info->cnode[wl->from].wl;
  sc->word_split_info->cnode[wl->from].wl = wl;

  /* デバッグプリント */
  if (anthy_splitter_debug_flags() & SPLITTER_DEBUG_WL) {
    anthy_print_word_list(sc, wl);
  }
}

struct word_list *
anthy_alloc_word_list(struct splitter_context *sc)
{
  struct word_list* wl;
  wl = anthy_smalloc(sc->word_split_info->WlAllocator);
  return wl;
}

/* 後続の活用語尾、助詞、助動詞を付ける */
static void
make_following_word_list(struct splitter_context *sc,
			 struct word_list *tmpl)
{
  /* このxsは自立語部の後続の文字列 */
  xstr xs;
  xs.str = sc->ce[tmpl->from+tmpl->len].c;
  xs.len = sc->char_count - tmpl->from - tmpl->len;
  tmpl->part[PART_DEPWORD].from =
    tmpl->part[PART_POSTFIX].from + tmpl->part[PART_POSTFIX].len;
  
  if (tmpl->node_id >= 0) {
    /* 普通のword_list */
    anthy_scan_node(sc, tmpl, &xs, tmpl->node_id);
  } else {
    /* 自立語がないword_list */
    struct wordseq_rule *r;
    struct word_list new_tmpl;
    new_tmpl = *tmpl;
    /* 名詞の後に続くルールに対して */
    for (r = gRules; r; r = r->next) {
      if (anthy_wtype_get_pos(r->wt) == POS_NOUN) {
	new_tmpl.part[PART_CORE].wt = r->wt;
	new_tmpl.core_wt_name = r->wt_name;
	new_tmpl.node_id = r->node_id;
	new_tmpl.head_pos = anthy_wtype_get_pos(new_tmpl.part[PART_CORE].wt);
	anthy_scan_node(sc, &new_tmpl, &xs, new_tmpl.node_id);
      }
    }
  }
}

static void
push_part_back(struct word_list *tmpl, int len,
	       seq_ent_t se, wtype_t wt)
{
  tmpl->len += len;
  tmpl->part[PART_POSTFIX].len += len;
  tmpl->part[PART_POSTFIX].wt = wt;
  tmpl->part[PART_POSTFIX].seq = se;
  tmpl->part[PART_POSTFIX].ratio = RATIO_BASE * 2 / 3;
  tmpl->last_part = PART_POSTFIX;
}

/* 接尾辞をくっつける */
static void 
make_suc_words(struct splitter_context *sc,
	       struct word_list *tmpl)
{
  int i, right;

  wtype_t core_wt = tmpl->part[PART_CORE].wt;
  /* 数詞、名前、サ変名詞のいずれかに付属語は付く */
  int core_is_num = 0;
  int core_is_name = 0;
  int core_is_sv_noun = 0;
  

  /* まず、接尾辞が付く自立語かチェックする */
  if (anthy_wtypecmp(anthy_wtype_num_noun, core_wt)) {
    core_is_num = 1;
  }
  if (anthy_wtypecmp(anthy_wtype_name_noun, core_wt)) {
    core_is_name = 1;
  }
  if (anthy_wtype_get_sv(core_wt)) {
    core_is_sv_noun = 1;
  }
  if (!core_is_num && !core_is_name && !core_is_sv_noun) {
    return ;
  }

  right = tmpl->part[PART_CORE].from + tmpl->part[PART_CORE].len;
  /* 自立語の右側の文字列に対して */
  for (i = 1;
       i <= sc->word_split_info->seq_len[right];
       i++){
    xstr xs;
    seq_ent_t suc;
    xs.str = sc->ce[right].c;
    xs.len = i;
    suc = anthy_get_seq_ent_from_xstr(&xs);
    if (anthy_get_seq_ent_pos(suc, POS_SUC)) {
      /* 右側の文字列は付属語なので、自立語の品詞にあわせてチェック */
      struct word_list new_tmpl;
      if (core_is_num &&
	  anthy_get_seq_ent_wtype_freq(suc, anthy_wtype_num_postfix)) {
	new_tmpl = *tmpl;
	push_part_back(&new_tmpl, i, suc, anthy_wtype_num_postfix);
	make_following_word_list(sc, &new_tmpl);
      }
      if (core_is_name &&
	  anthy_get_seq_ent_wtype_freq(suc, anthy_wtype_name_postfix)) {
	new_tmpl = *tmpl;
	push_part_back(&new_tmpl, i, suc, anthy_wtype_name_postfix);
	new_tmpl.weak_len += i;
	make_following_word_list(sc, &new_tmpl);
      }
      if (core_is_sv_noun &&
	  anthy_get_seq_ent_wtype_freq(suc, anthy_wtype_sv_postfix)) {
	new_tmpl = *tmpl;
	push_part_back(&new_tmpl, i, suc, anthy_wtype_sv_postfix);
	new_tmpl.weak_len += i;
	make_following_word_list(sc, &new_tmpl);
      }
    }
  }
}

static void
push_part_front(struct word_list *tmpl, int len,
		seq_ent_t se, wtype_t wt)
{
  tmpl->from = tmpl->from - len;
  tmpl->len = tmpl->len + len;
  tmpl->part[PART_PREFIX].from = tmpl->from;
  tmpl->part[PART_PREFIX].len += len;
  tmpl->part[PART_PREFIX].wt = wt;
  tmpl->part[PART_PREFIX].seq = se;
  tmpl->part[PART_PREFIX].ratio = RATIO_BASE * 2 / 3;
}

/* 接頭辞をくっつけてから接尾辞をくっつける */
static void
make_pre_words(struct splitter_context *sc,
	       struct word_list *tmpl)
{
  int i;
  wtype_t core_wt = tmpl->part[PART_CORE].wt;
  /* 自立語は数詞か？ */
  if (!anthy_wtypecmp(anthy_wtype_num_noun, core_wt)) {
    return ;
  }
  /* 接頭辞を列挙する */
  for (i = 1; 
       i <= sc->word_split_info->rev_seq_len[tmpl->part[PART_CORE].from];
       i++) {
    seq_ent_t pre;
    /* このxsは自立語部の前の文字列 */
    xstr xs;
    xs.str = sc->ce[tmpl->part[PART_CORE].from - i].c;
    xs.len = i;
    pre = anthy_get_seq_ent_from_xstr(&xs);
    if (anthy_get_seq_ent_pos(pre, POS_PRE)) {
      struct word_list new_tmpl;
      if (anthy_get_seq_ent_wtype_freq(pre, anthy_wtype_num_prefix)) {
	new_tmpl = *tmpl;
	push_part_front(&new_tmpl, i, pre, anthy_wtype_num_prefix);
	make_following_word_list(sc, &new_tmpl);
	/* 数の場合は接尾辞もくっつける */
	make_suc_words(sc, &new_tmpl);
      }
    }
  }
}

/* wordlistを初期化する */
static void
setup_word_list(struct word_list *wl, int from, int len, int is_compound)
{
  int i;
  wl->from = from;
  wl->len = len;
  wl->weak_len = 0;
  wl->is_compound = is_compound;
  /* partの配列を初期化する */
  for (i = 0; i < NR_PARTS; i++) {
    wl->part[i].from = 0;
    wl->part[i].len = 0;
    wl->part[i].wt = anthy_wt_none;
    wl->part[i].seq = 0;
    wl->part[i].freq = 1;/* 頻度の低い単語としておく */
    wl->part[i].ratio = RATIO_BASE;
    wl->part[i].dc = DEP_RAW;
  }
  /* 自立語のパートを設定 */
  wl->part[PART_CORE].from = from;
  wl->part[PART_CORE].len = len;
  /**/
  wl->score = 0;
  wl->node_id = -1;
  wl->last_part = PART_CORE;
  wl->head_pos = POS_NONE;
  wl->tail_ct = CT_NONE;
  /**/
  wl->core_wt_name = NULL;
}

/*
 * ある独立語に対して、接頭辞、接尾辞、付属語を付けたものを
 * 文節の候補(=word_list)としてcacheに追加する
 */
static void
make_word_list(struct splitter_context *sc,
	       seq_ent_t se,
	       int from, int len,
	       int is_compound)
{
  struct word_list tmpl;
  struct wordseq_rule *r;

  /* テンプレートの初期化 */
  setup_word_list(&tmpl, from, len, is_compound);
  tmpl.part[PART_CORE].seq = se;

  /* 各ルールにマッチするか比較 */
  for (r = gRules; r; r = r->next) {
    int freq = anthy_get_seq_ent_wtype_freq(se, r->wt);
    if (freq) {
      /* 自立語の品詞はそのルールにあっている */
      if (anthy_splitter_debug_flags() & SPLITTER_DEBUG_ID) {
	/* 品詞表のデバッグ用*/
	xstr xs;
	xs.str = sc->ce[tmpl.part[PART_CORE].from].c;
	xs.len = tmpl.part[PART_CORE].len;
	anthy_putxstr(&xs);
	printf(" name=%s freq=%d node_id=%d\n", r->wt_name, freq, r->node_id);
      }
      /* 遷移したルールの情報を転記する */
      tmpl.part[PART_CORE].wt = r->wt;
      tmpl.part[PART_CORE].ratio = r->ratio;
      tmpl.part[PART_CORE].freq = freq;
      tmpl.core_wt_name = r->wt_name;
      tmpl.node_id = r->node_id;
      tmpl.head_pos = anthy_wtype_get_pos(tmpl.part[PART_CORE].wt);
      /**/
      tmpl.part[PART_POSTFIX].from =
	tmpl.part[PART_CORE].from +
	tmpl.part[PART_CORE].len;
      /**/
      if (anthy_wtype_get_pos(r->wt) == POS_NOUN ||
	  anthy_wtype_get_pos(r->wt) == POS_NUMBER) {
	/* 接頭辞、接尾辞は名詞、数詞にしか付かないことにしている */
	make_pre_words(sc, &tmpl);
	make_suc_words(sc, &tmpl);
      }
      /* 接頭辞、接尾辞無しで助詞助動詞をつける */
      make_following_word_list(sc, &tmpl);
    }
  }
}

static void
make_dummy_head(struct splitter_context *sc)
{
  struct word_list tmpl;
  setup_word_list(&tmpl, 0, 0, 0);
  tmpl.part[PART_CORE].seq = 0;
  tmpl.part[PART_CORE].wt = anthy_wtype_noun;
  tmpl.score = 0;
  tmpl.head_pos = anthy_wtype_get_pos(tmpl.part[PART_CORE].wt);
  make_suc_words(sc, &tmpl);
}


static int
is_indep(seq_ent_t se)
{
  if (!se) {
    return 0;
  }
  return anthy_get_seq_ent_indep(se);
}

/* コンテキストの文字列中の全てのword_listを列挙する */
void 
anthy_make_word_list_all(struct splitter_context *sc)
{
  int i, j;
  xstr xs;
  seq_ent_t se;
  struct depword_ent {
    struct depword_ent *next;
    int from, len;
    int is_compound;
    seq_ent_t se;
  } *head, *de;
  struct word_split_info_cache *info;
  allocator de_ator;

  info = sc->word_split_info;
  head = 0;
  de_ator = anthy_create_allocator(sizeof(struct depword_ent), 0);

  /* 全ての自立語を列挙 */
  /* 開始地点のループ */
  for (i = 0; i < sc->char_count ; i++) {
    int search_len = sc->char_count - i;
    int search_from = 0;
    if (search_len > 30) {
      search_len = 30;
    }

    /* 文字列長のループ(長い方から) */
    for (j = search_len; j > search_from; j--) {
      xs.len = j;
      xs.str = sc->ce[i].c;

      se = anthy_get_seq_ent_from_xstr(&xs);

      if (se) {
	/* 各、部分文字列が単語ならば接頭辞、接尾辞の
	   最大長を調べてマークする */
	if (j > info->seq_len[i] &&
	    anthy_get_seq_ent_pos(se, POS_SUC)) {
	  info->seq_len[i] = j;
	}
	if (j > info->rev_seq_len[i + j] &&
	    anthy_get_seq_ent_pos(se, POS_PRE)) {
	  info->rev_seq_len[i + j] = j;
	}
      }

      /* 発見した自立語をリストに追加 */
      if (is_indep(se) || anthy_get_nr_compound_ents(se) > 0) {
	de = (struct depword_ent *)anthy_smalloc(de_ator);
	de->from = i;
	de->len = j;
	de->se = se;
	if (anthy_get_nr_compound_ents(se) > 0) {
	  de->is_compound = 1;
	} else {
	  de->is_compound = 0;
	}

	de->next = head;
	head = de;
      }
    }
  }

  /* 発見した自立語全てに対して付属語パターンの検索 */
  for (de = head; de; de = de->next) {
    make_word_list(sc, de->se, de->from, de->len, de->is_compound);
  }
  

    /* 自立語の無いword_list */
  for (i = 0; i < sc->char_count; i++) {
    struct word_list tmpl;
    if (i == 0) {
      setup_word_list(&tmpl, i, 0, 0);
      make_following_word_list(sc, &tmpl);
    } else {
      int type = anthy_get_xchar_type(*sc->ce[i - 1].c);
      if (type & XCT_CLOSE || type & XCT_SYMBOL) {
	setup_word_list(&tmpl, i, 0, 0);
	make_following_word_list(sc, &tmpl);
      }
    }
  }  

  /* 先頭に0文字の自立語を付ける */
  make_dummy_head(sc);

  anthy_free_allocator(de_ator);
}

static void
parse_line(char **tokens, int nr)
{
  struct wordseq_rule *r;
  int tmp;
  if (nr < 2) {
    printf("Syntex error in indepword defs"
	   " :%d.\n", anthy_get_line_number());
    return ;
  }
  /* 行の先頭には品詞の名前が入っている */
  r = anthy_smalloc(wordseq_rule_ator);
  r->wt_name = anthy_name_intern(tokens[0]);
  anthy_init_wtype_by_name(tokens[0], &r->wt);

  /* 比率 */
  tmp = atoi(tokens[1]);
  if (tmp == 0) {
    tmp = 1;
  }
  r->ratio = RATIO_BASE - tmp*(RATIO_BASE/64);

  /* その次にはノード名が入っている */
  r->node_id = anthy_get_node_id_by_name(tokens[2]);

  /* ルールを追加 */
  r->next = gRules;
  gRules = r;
}

/** 付属語グラフを読み込む */
static int 
init_word_seq_tab(void)
{
  const char *fn;
  char **tokens;
  int nr;

  wordseq_rule_ator = anthy_create_allocator(sizeof(struct wordseq_rule),
					     NULL);

  fn = anthy_conf_get_str("INDEPWORD");
  if (!fn){
    printf("independent word dict unspecified.\n");
    return -1;
  }
  if (anthy_open_file(fn) == -1) {
    printf("Failed to open indep word dict (%s).\n", fn);
    return -1;
  }
  /* ファイルを一行ずつ読む */
  gRules = NULL;
  while (!anthy_read_line(&tokens, &nr)) {
    parse_line(tokens, nr);
    anthy_free_line();
  }
  anthy_close_file();

  return 0;
}


int
anthy_init_wordlist(void)
{
  return init_word_seq_tab();
}

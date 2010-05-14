/*
 * HMMとビタビアルゴリズム(viterbi algoritgm)によって
 * 文節の区切りを決定してマークする。
 *
 * 外部から呼び出される関数
 *  anthy_hmm()
 *
 * Copyright (C) 2004-2006 YOSHIDA Yuichi
 * Copyright (C) 2006 TABATA Yusuke
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
 * コンテキスト中に存在するmeta_wordをつないでグラフを構成します。
 * (このグラフのことをラティス(lattice/束)もしくはトレリス(trellis)と呼びます)
 * meta_wordどうしの接続がグラフのノードとなり、構造体hmm_nodeの
 * リンクとして構成されます。
 *
 * ここでの処理は次の二つの要素で構成されます
 * (1) グラフを構成しつつ、各ノードへの到達確率を求める
 * (2) グラフを後ろ(右)からたどって最適なパスを求める
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <alloc.h>
#include <xstr.h>
#include <segclass.h>
#include <splitter.h>
#include <parameter.h>
#include "wordborder.h"

float anthy_normal_length = 20.0; /* 文節の期待される長さ */

#define NODE_MAX_SIZE 50

struct feature_list {
  /* いまのところ、素性は一個 */
  int index;
};

/* グラフのノード(遷移状態) */
struct hmm_node {
  int border; /* 文字列中のどこから始まる状態か */
  int nth; /* 現在いくつめの文節か */
  enum seg_class seg_class; /* この状態の品詞 */


  double real_probability;  /* ここに至るまでの確率(文節数補正無し) */
  double probability;  /* ここに至るまでの確率(文節数補正有り) */
  int score_sum; /* 使用しているmetawordのスコアの合計*/


  struct hmm_node* before_node; /* 一つ前の遷移状態 */
  struct meta_word* mw; /* この遷移状態に対応するmeta_word */

  struct hmm_node* next; /* リスト構造のためのポインタ */
};

struct node_list_head {
  struct hmm_node *head;
  int nr_nodes;
};

struct hmm_info {
  /* 遷移状態のリストの配列 */
  struct node_list_head *hmm_node_list;
  struct splitter_context *sc;
  /* HMMノードのアロケータ */
  allocator node_allocator;
};

/* 素性の重みの行列 */
static double g_transition[SEG_SIZE*SEG_SIZE] = {
#include "transition.h"
};


static void
print_hmm_node(struct hmm_info *info, struct hmm_node *node)
{
  if (!node) {
    printf("**hmm_node (null)*\n");
    return ;
  }
  printf("**hmm_node score_sum=%d, nth=%d*\n", node->score_sum, node->nth);
  printf("probability=%.128f\n", node->real_probability);
  if (node->mw) {
    anthy_print_metaword(info->sc, node->mw);
  }
}

static double
get_poisson(double lambda, int r)
{
  int i;
  double result;

  /* 要するにポワソン分布 */
  result = pow(lambda, r) * exp(-lambda);
  for (i = 2; i <= r; ++i) {
    result /= i;
  }

  return result;
}

/* 文節の形式からスコアを調整する */
static double
get_form_bias(struct hmm_node *node)
{
  struct meta_word *mw = node->mw;
  double bias;
  int wrap = 0;
  /* wrapされている場合は内部のを使う */
  while (mw->type == MW_WRAP) {
    mw = mw->mw1;
    wrap = 1;
  }
  /* 文節長による調整 */
  bias = get_poisson(anthy_normal_length, mw->len);
  /* 付属語の頻度による調整 */
  bias *= (mw->dep_score + 1000);
  bias /= 2000;
  return bias;
}

static void
fill_bigram_feature(struct feature_list *features, int from, int to)
{
  features->index = from * SEG_SIZE + to;
}

static void
build_feature_list(struct hmm_node *node, struct feature_list *features)
{
  fill_bigram_feature(features, node->before_node->seg_class,
		      node->seg_class);
}

static double
calc_probability(struct feature_list *features)
{
  return g_transition[features->index];
}

static double
get_transition_probability(struct hmm_node *node)
{
  struct feature_list features;
  double probability;
  if (anthy_seg_class_is_depword(node->seg_class)) {
    /* 付属語のみの場合 */
    return probability = 1.0 / SEG_SIZE;
  } else if (node->seg_class == SEG_FUKUSHI || 
	     node->seg_class == SEG_RENTAISHI) {
    /* 副詞、連体詞 */
    probability = 2.0 / SEG_SIZE;
  } else if (node->before_node->seg_class == SEG_HEAD &&
	     (node->seg_class == SEG_SETSUZOKUGO)) {
    /* 文頭 -> 接続語 */
    probability = 1.0 / SEG_SIZE;
  } else if (node->mw && node->mw->type == MW_NOUN_NOUN_PREFIX) {
    /* 名詞接頭辞と名詞を付けたmeta_word */
    build_feature_list(node, &features);
    probability = calc_probability(&features);
    /**/
    fill_bigram_feature(&features, SEG_MEISHI, SEG_MEISHI);
    probability *= calc_probability(&features);
  } else {
    /* その他の場合 */
    build_feature_list(node, &features);
    probability = calc_probability(&features);
  }
  /* 文節の形に対する評価 */
  probability *= get_form_bias(node);
  return probability;
}

static struct hmm_info*
alloc_hmm_info(struct splitter_context *sc, int size)
{
  int i;
  struct hmm_info* info = (struct hmm_info*)malloc(sizeof(struct hmm_info));
  info->sc = sc;
  info->hmm_node_list = (struct node_list_head*)
    malloc((size + 1) * sizeof(struct node_list_head));
  for (i = 0; i < size + 1; i++) {
    info->hmm_node_list[i].head = NULL;
    info->hmm_node_list[i].nr_nodes = 0;
  }
  info->node_allocator = anthy_create_allocator(sizeof(struct hmm_node), NULL);
  return info;
}

static void
calc_node_parameters(struct hmm_node *node)
{
  
  /* 対応するmetawordが無い場合は文頭と判断する */
  node->seg_class = node->mw ? node->mw->seg_class : SEG_HEAD; 

  if (node->before_node) {
    /* 左に隣接するノードがある場合 */
    node->nth = node->before_node->nth + 1;
    node->score_sum = node->before_node->score_sum +
      (node->mw ? node->mw->score : 0);
    node->real_probability = node->before_node->real_probability *
      get_transition_probability(node);
    node->probability = node->real_probability * node->score_sum;
  } else {
    /* 左に隣接するノードが無い場合 */
    node->nth = 0;
    node->score_sum = 0;
    node->real_probability = 1.0;
    node->probability = node->real_probability;
  }
}

static struct hmm_node*
alloc_hmm_node(struct hmm_info *info, struct hmm_node* before_node,
	       struct meta_word* mw, int border)
{
  struct hmm_node* node;
  node = anthy_smalloc(info->node_allocator);
  node->before_node = before_node;
  node->border = border;
  node->next = NULL;
  node->mw = mw;

  calc_node_parameters(node);

  return node;
}

static void 
release_hmm_node(struct hmm_info *info, struct hmm_node* node)
{
  anthy_sfree(info->node_allocator, node);
}

static void
release_hmm_info(struct hmm_info* info)
{
  anthy_free_allocator(info->node_allocator);
  free(info->hmm_node_list);
  free(info);
}

static int
cmp_node_by_type(struct hmm_node *lhs, struct hmm_node *rhs,
		 enum metaword_type type)
{
  if (lhs->mw->type == type && rhs->mw->type != type) {
    return 1;
  } else if (lhs->mw->type != type && rhs->mw->type == type) {
    return -1;
  } else {
    return 0;
  }
}

static int
cmp_node_by_type_to_type(struct hmm_node *lhs, struct hmm_node *rhs,
			 enum metaword_type type1, enum metaword_type type2)
{
  if (lhs->mw->type == type1 && rhs->mw->type == type2) {
    return 1;
  } else if (lhs->mw->type == type2 && rhs->mw->type == type1) {
    return -1;
  } else {
    return 0;
  } 
}

/*
 * ノードを比較する
 *
 ** 返り値
 * 1: lhsの方が確率が高い
 * 0: 同じ
 * -1: rhsの方が確率が高い
 */
static int
cmp_node(struct hmm_node *lhs, struct hmm_node *rhs)
{
  struct hmm_node *lhs_before = lhs;
  struct hmm_node *rhs_before = rhs;
  int ret;

  if (lhs && !rhs) return 1;
  if (!lhs && rhs) return -1;
  if (!lhs && !rhs) return 0;

  while (lhs_before && rhs_before) {
    if (lhs_before->mw && rhs_before->mw &&
	lhs_before->mw->from + lhs_before->mw->len == rhs_before->mw->from + rhs_before->mw->len) {
      /* 学習から作られたノードかどうかを見る */
      ret = cmp_node_by_type(lhs_before, rhs_before, MW_OCHAIRE);
      if (ret != 0) return ret;
      /* ラップされたものは確率が低い */
      /*
	ret = cmp_node_by_type(lhs, rhs, MW_WRAP);
	if (ret != 0) return -ret;
      */

      /* COMPOUND_PARTよりはCOMPOUND_HEADを優先 */
      ret = cmp_node_by_type_to_type(lhs_before, rhs_before, MW_COMPOUND_HEAD, MW_COMPOUND_PART);
      if (ret != 0) return ret;
    } else {
      break;
    }
    lhs_before = lhs_before->before_node;
    rhs_before = rhs_before->before_node;
  }

  /* 最後に遷移確率を見る */
  if (lhs->probability > rhs->probability) {
    return 1;
  } else if (lhs->probability < rhs->probability) {
    return -1;
  } else {
    return 0;
  }
}

/*
 * 構成中のラティスにノードを追加する
 */
static void
push_node(struct hmm_info* info, struct hmm_node* new_node, int position)
{
  struct hmm_node* node;
  struct hmm_node* previous_node = NULL;

  if (anthy_splitter_debug_flags() & SPLITTER_DEBUG_HM) {
    print_hmm_node(info, new_node);
  }

  /* 先頭のnodeが無ければ無条件に追加 */
  node = info->hmm_node_list[position].head;
  if (!node) {
    info->hmm_node_list[position].head = new_node;
    info->hmm_node_list[position].nr_nodes ++;
    return;
  }

  while (node->next) {
    /* 余計なノードを追加しないための枝刈り */
    if (new_node->seg_class == node->seg_class &&
	new_node->border == node->border) {
      /* segclassが同じで、始まる位置が同じなら */
      switch (cmp_node(new_node, node)) {
      case 0:
      case 1:
	/* 新しい方が確率が大きいか学習によるものなら、古いのと置き換え*/
	if (previous_node) {
	  previous_node->next = new_node;
	} else {
	  info->hmm_node_list[position].head = new_node;
	}
	new_node->next = node->next;
	release_hmm_node(info, node);
	break;
      case -1:
	/* そうでないなら削除 */
	release_hmm_node(info, new_node);
	break;
      }
      return;
    }
    previous_node = node;
    node = node->next;
  }
  /* 最後のノードの後ろに追加 */
  node->next = new_node;
  info->hmm_node_list[position].nr_nodes ++;
}

/* 一番確率の低いノードを消去する*/
static void
remove_min_node(struct hmm_info *info, struct node_list_head *node_list)
{
  struct hmm_node* node = node_list->head;
  struct hmm_node* previous_node = NULL;
  struct hmm_node* min_node = node;
  struct hmm_node* previous_min_node = NULL;

  /* 一番確率の低いノードを探す */
  while (node) {
    if (cmp_node(node, min_node) < 0) {
      previous_min_node = previous_node;
      min_node = node;
    }
    previous_node = node;
    node = node->next;
  }

  /* 一番確率の低いノードを削除する */
  if (previous_min_node) {
    previous_min_node->next = min_node->next;
  } else {
    node_list->head = min_node->next;
  }
  release_hmm_node(info, min_node);
  node_list->nr_nodes --;
}

/* いわゆるビタビアルゴリズムを使用して経路を選ぶ */
static void
choose_path(struct hmm_info* info, int to)
{
  /* 最後まで到達した遷移のなかで一番確率の大きいものを選ぶ */
  struct hmm_node* node;
  struct hmm_node* best_node = NULL;
  int last = to; 
  while (!info->hmm_node_list[last].head) {
    /* 最後の文字まで遷移していなかったら後戻り */
    --last;
  }
  for (node = info->hmm_node_list[last].head; node; node = node->next) {
    if (cmp_node(node, best_node) > 0) {
      best_node = node;
    }
  }
  if (!best_node) {
    return;
  }

  /* 遷移を逆にたどりつつ文節の切れ目を記録 */
  node = best_node;
  while (node->before_node) {
    info->sc->word_split_info->best_seg_class[node->border] =
      node->seg_class;
    anthy_mark_border_by_metaword(info->sc, node->mw);
    node = node->before_node;
  }
}

static void
build_hmm_graph(struct hmm_info* info, int from, int to)
{
  int i;
  struct hmm_node* node;
  struct hmm_node* left_node;

  /* 始点となるノードを追加 */
  node = alloc_hmm_node(info, NULL, NULL, from);
  push_node(info, node, from);

  /* info->hmm_list[index]にはindexまでの遷移が入っているのであって、
   * indexからの遷移が入っているのではない 
   */

  /* 全ての遷移を左から試す */
  for (i = from; i < to; ++i) {
    for (left_node = info->hmm_node_list[i].head; left_node;
	 left_node = left_node->next) {
      struct meta_word *mw;
      /* i文字目に到達するhmm_nodeのループ */

      for (mw = info->sc->word_split_info->cnode[i].mw; mw; mw = mw->next) {
	int position;
	struct hmm_node* new_node;
	/* i文字目からのmeta_wordのループ */

	if (mw->can_use != ok) {
	  continue; /* 決められた文節の区切りをまたぐmetawordは使わない */
	}
	position = i + mw->len;
	new_node = alloc_hmm_node(info, left_node, mw, i);
	push_node(info, new_node, position);

	/* 解の候補が多すぎたら、確率の低い方から削る */
	if (info->hmm_node_list[position].nr_nodes >= NODE_MAX_SIZE) {
	  remove_min_node(info, &info->hmm_node_list[position]);
	}
      }
    }
  }

  /* 文末補正 */
  for (node = info->hmm_node_list[to].head; node; node = node->next) {
    struct feature_list features;
    fill_bigram_feature(&features, node->seg_class, SEG_TAIL);
    node->probability = node->probability *
      calc_probability(&features);
  }
}

void
anthy_hmm(struct splitter_context *sc, int from, int to)
{
  struct hmm_info* info = alloc_hmm_info(sc, to);
  build_hmm_graph(info, from, to);
  choose_path(info, to);
  release_hmm_info(info);
}


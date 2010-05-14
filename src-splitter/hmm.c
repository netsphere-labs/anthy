#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <xstr.h>
#include <segclass.h>
#include <splitter.h>
#include <parameter.h>
#include "wordborder.h"

float anthy_normal_length = 20.0; /* 文節の期待される長さ */

#define NODE_MAX_SIZE 100

/* 各遷移状態 */
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

struct hmm_info {
  int size;
  struct hmm_node** hmm_list; /* 遷移状態のリストの配列 */
};

double g_transition[SEG_SIZE][SEG_SIZE] = {
#include "transition.h"
};


static void
print_hmm_node(struct splitter_context *sc, struct hmm_node *node)
{
  printf("**hmm_node score_sum=%d, nth=%d*\n", node->score_sum, node->nth);
  printf("probability=%.255f\n", node->real_probability);
  anthy_print_metaword(sc, node->mw);
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

static double
get_probability(struct hmm_node *node)
{
  double probability;
  if (anthy_seg_class_is_depword(node->seg_class)) {
    probability = 1.0 / SEG_SIZE * 2;
  } else if (node->before_node->seg_class == SEG_HEAD &&
	     (node->seg_class == SEG_FUKUSHI)) {
    probability = 1.0 / SEG_SIZE * 2;
  } else {
    probability = g_transition[node->before_node->seg_class][node->seg_class]
      * get_poisson(anthy_normal_length, node->mw->len - node->mw->weak_len);
  }
  return probability;
}

static struct hmm_info*
alloc_hmm_info(int size)
{
  struct hmm_info* info = (struct hmm_info*)malloc(sizeof(struct hmm_info));
  info->size = size + 1;
  info->hmm_list = (struct hmm_node**)calloc(size, sizeof(struct hmm_node*) * (info->size));
  return info;
}

static struct hmm_node*
alloc_hmm_node(struct hmm_node* before_node, struct meta_word* mw, int border)
{
  struct hmm_node* node;
  node = (struct hmm_node*)malloc(sizeof(struct hmm_node));
  node->before_node = before_node;
  node->border = border;
  node->next = NULL;
  node->mw = mw;
  
  /* 対応するmetawordが無い場合は文頭と判断する */
  node->seg_class = mw ? mw->seg_class : SEG_HEAD; 

  if (before_node) {
    node->nth = before_node->nth + 1;
    node->score_sum = before_node->score_sum + mw ? mw->score : 0;
    node->real_probability = before_node->real_probability * get_probability(node);
    node->probability = node->real_probability * node->score_sum;
  } else {
    node->nth = 0;
    node->score_sum = 0;
    node->real_probability = 1.0;
    node->probability = node->real_probability;
  }
  return node;
}

static void 
release_hmm_node(struct hmm_node* node)
{
  free(node);
}

static void
release_hmm_info(struct hmm_info* info)
{
  int i;
  for (i = 0; i < info->size; ++i) {  
    struct hmm_node* node;
    struct hmm_node* next;

    node = info->hmm_list[i];
    while (node) {
      next = node->next;
      release_hmm_node(node);
      node = next;
    }
  }
  free(info->hmm_list);
  free(info);
}

static int
cmp_node_by_type(struct hmm_node *lhs, struct hmm_node *rhs, enum metaword_type type)
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
cmp_node(struct hmm_node *lhs, struct hmm_node *rhs)
{
  struct hmm_node *lhs_before;
  struct hmm_node *rhs_before;
  int ret;

  if (lhs && !rhs) return 1;
  if (!lhs && rhs) return -1;
  if (!lhs && !rhs) return 0;

  /* まず、学習から作られたノードかどうかを見る */
  ret = cmp_node_by_type(lhs, rhs, MW_OCHAIRE);
  if (ret != 0) return ret;
  ret = cmp_node_by_type(lhs, rhs, MW_COMPOUND_HEAD);
  if (ret != 0) return ret;

  lhs_before = lhs->before_node;
  rhs_before = rhs->before_node;
  if (lhs_before && rhs_before && lhs_before->mw && rhs_before->mw) {
    ret = cmp_node_by_type(lhs_before, rhs_before, MW_OCHAIRE);
    if (ret != 0) return ret;
  }
  
  /* 次に遷移確率を見る */
  if (lhs->probability > rhs->probability) {
    return 1;
  } else if (lhs->probability < rhs->probability) {
    return -1;
  } else {
    return 0;
  }
}

static void
push_node(struct hmm_info* info, struct hmm_node* new_node, int position)
{
  struct hmm_node* node = info->hmm_list[position];
  struct hmm_node* previous_node = NULL;

  if (!node) {
    info->hmm_list[position] = new_node;
    return;
  }

  while (node->next) {
    /* 枝刈り */
    if (new_node->seg_class == node->seg_class && new_node->border == node->border) {
      /* segclassが同じで、始まる位置が同じなら */
      switch (cmp_node(new_node, node)) {
      case 0:
      case 1:
	/* 新しい方が確率が大きいか学習によるものなら、古いのと置き換え*/
	if (previous_node) {
	  previous_node->next = new_node;
	} else {
	  info->hmm_list[position] = new_node;
	}
	new_node->next = node->next;
	release_hmm_node(node);
	break;
      case -1:
	/* そうでないなら削除 */
	release_hmm_node(new_node);
	break;
      }
      return;
    }
    previous_node = node;
    node = node->next;
  }
  node->next = new_node;
}

static int
list_length(struct hmm_info* info, int position)
{
  int length = 1;
  struct hmm_node* node = info->hmm_list[position];
  while(node) {
    ++length;
    node = node->next;
  }
  return length;
}

static void
remove_min_node(struct hmm_info* info, int position)
{
  struct hmm_node* node = info->hmm_list[position];
  struct hmm_node* previous_node = NULL;
  struct hmm_node* min_node = node;
  struct hmm_node* previous_min_node = NULL;
  int list_len = 0;

  /* 一番確立の低いノードを消去する*/
  while (node) {
    if (cmp_node(node, min_node) < 0) {
      previous_min_node = previous_node;
      min_node = node;
    }
    previous_node = node;
    node = node->next;
    ++list_len;
  }
  if (previous_min_node) {
    previous_min_node->next = min_node->next;
  } else {
    info->hmm_list[position] = min_node->next;
  }
  release_hmm_node(min_node);
}

static void
hmm(struct splitter_context *sc, int from, int to, struct hmm_info* info)
{
  int i;
  struct hmm_node* node;
  struct hmm_node* first_node;  

  first_node = alloc_hmm_node(NULL, NULL, from);
  push_node(info, first_node, from);

  /* info->hmm_list[index]にはindexまでの遷移が入っているのであって、
     indexからの遷移が入っているのではない */

  /* 全ての遷移を試す */
  for (i = from; i < to; ++i) {
    for (node = info->hmm_list[i]; node; node = node->next) {
      struct meta_word *mw;
      for (mw = sc->word_split_info->cnode[i].mw; mw; mw = mw->next) {
	int position;
	struct hmm_node* new_node;
	if (mw->can_use != ok) continue; /* 決められた文節の区切りをまたぐmetawordは使わない */
	position = i + mw->len;
	new_node = alloc_hmm_node(node, mw, i);
	push_node(info, new_node, position);

	/* 解の候補が多すぎたら、確率の低い方から削る */
	if (list_length(info, position) >= NODE_MAX_SIZE) {
	  remove_min_node(info, position);
	}
      }
    }
  }

  /* 文末補正 */
  for (node = info->hmm_list[to]; node; node = node->next) {
    node->probability = node->probability * g_transition[node->seg_class][SEG_TAIL];
    if (node->nth == 1) {
      if (node->seg_class == SEG_MEISHI) {
	node->probability *= 5;
      } else {
	node->probability /= 5;
      }
    }
  }

  {
    /* 最後まで到達した遷移のなかで一番確率の大きいものを選ぶ */
    struct hmm_node* best_node = NULL;
    for (node = info->hmm_list[to]; node; node = node->next) {
      if (cmp_node(node, best_node) > 0) {
	best_node = node;
      }
    }
    if (!best_node) return;

    /* 遷移を逆にたどりつつ文節の切れ目を記録 */
    node = best_node;
    while (node->before_node) {
      int i;
      enum metaword_type type = node->mw->type;
      if (type == MW_COMPOUND_HEAD || type == MW_COMPOUND) {
	for (i = 1; i < node->mw->len; ++i) {
	  sc->word_split_info->seg_border[node->border + i] = 0;
	}
      }
      sc->word_split_info->seg_border[node->border] = 1;
      sc->word_split_info->best_seg_class[node->border] = node->seg_class;
      node = node->before_node;
    }
  }
}

void
anthy_hmm(struct splitter_context *sc, int from, int to)
{
  struct hmm_info* info = alloc_hmm_info(to);
  hmm(sc, from, to, info);
  release_hmm_info(info);
}


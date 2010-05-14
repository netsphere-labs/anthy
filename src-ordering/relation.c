/*
 * 文節の関係を処理する
 * Copyright (C) 2002-2004 TABATA Yusuke
 */

#include <stdlib.h>

#include <segclass.h>
#include <segment.h>
#include <ordering.h>
#include <dic.h>
#include <hash_map.h>
#include "sorter.h"

static void
reorder_candidate(int from_word_id, struct seg_ent *seg)
{
  int i, pos;
  struct cand_ent *ce = seg->cands[0];
  if (ce->core_elm_index == -1) {
    return ;
  }
  /* 0番目の候補の品詞 */
  pos = anthy_wtype_get_pos(ce->elm[ce->core_elm_index].wt);

  for (i = 0; i < seg->nr_cands; i++) {
    int word_id;
    ce = seg->cands[i];
    if (ce->core_elm_index == -1) {
      continue;
    }
    word_id = ce->elm[ce->core_elm_index].id;
    if (anthy_dic_check_word_relation(from_word_id, word_id) &&
	anthy_wtype_get_pos(ce->elm[ce->core_elm_index].wt) == pos) {
      /* 用例にマッチしたので、候補のスコアを更新 */
      ce->flag |= CEF_USEDICT;
      ce->score *= 10;
    }
  }
}

/*
 * 用例を用いて候補を並び替える
 *  @nth番目以降の文節を対象とする
 */
void
anthy_reorder_candidates_by_relation(struct segment_list *sl, int nth)
{
  int i;
  for (i = nth; i < sl->nr_segments; i++) {
    int j;
    struct seg_ent *cur_seg;
    struct cand_ent *ce;
    int word_id;
    cur_seg = anthy_get_nth_segment(sl, i);
    if (cur_seg->cands[0]->core_elm_index == -1) {
      /* 一番目の候補がseq_entから作られた候補ではない */
      continue;
    }
    ce = cur_seg->cands[0];
    word_id = ce->elm[ce->core_elm_index].id;
    if (word_id == -1) {
      /**/
      continue;
    }
    /* 近所の文節を順に見ていく */
    for (j = i - 2; j < i + 2 && j < sl->nr_segments; j++) {
      struct seg_ent *target_seg;
      if (j < 0 || j == i) {
	continue;
      }
      /* i番目の文節と前後のj番目の文節に対して */
      target_seg = anthy_get_nth_segment(sl, j);
      reorder_candidate(word_id, target_seg);
    }
  }
}

void
anthy_init_ordering_context(struct segment_list *sl,
			    struct ordering_context_wrapper *cw)
{
  (void)sl;
  (void)cw;
}

void
anthy_release_ordering_context(struct segment_list *sl,
			       struct ordering_context_wrapper *cw)
{
  (void)sl;
  (void)cw;
}

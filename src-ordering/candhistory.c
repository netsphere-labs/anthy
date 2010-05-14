/*
 * 候補の履歴を覚える
 *
 *
 * ある読みの履歴が 候補A 候補B 候補A 候補A 候補A
 * であったというような情報をもとに候補のスコアを加点する。
 *
 */
#include <segment.h>
#include <record.h>
#include "sorter.h"

#define HISTORY_DEPTH 8
#define MAX_HISTORY_ENTRY 200

/** 文節のコミットを履歴に追加する */
static void
learn_history(struct seg_ent *seg)
{
  int nr, i;

  if (anthy_select_row(&seg->str, 1)) {
    return ;
  }
  /* シフトする */
  nr = anthy_get_nr_values();
  nr ++;
  if (nr > HISTORY_DEPTH) {
    nr = HISTORY_DEPTH;
  }
  for (i = nr - 1; i > 0; i--) {
    xstr *xs = anthy_get_nth_xstr(i - 1);
    anthy_set_nth_xstr(i, xs);
  }
  /* 0番目に設定 */
  anthy_set_nth_xstr(0, &seg->cands[seg->committed]->str);
  anthy_mark_row_used();
}

/** 外から呼ばれる関数 
 * 履歴に追加する */
void
anthy_learn_cand_history(struct segment_list *sl)
{
  int i, nr = 0;
  if (anthy_select_section("CAND_HISTORY", 1)) {
    return ;
  }
  for (i = 0; i < sl->nr_segments; i++) {
    struct seg_ent *seg = anthy_get_nth_segment(sl, i);
    xstr *xs = &seg->str;
    if (seg->committed < 0) {
      continue;
    }
    if (anthy_select_row(xs, 0)) {
      if (seg->committed == 0) {
	/* 候補のエントリが無くて、コミットされた候補も先頭のものであればパス */
	continue;
      }
    }
    /**/
    learn_history(seg);
    nr ++;
  }
  if (nr) {
    anthy_truncate_section(MAX_HISTORY_ENTRY);
  }
}

/* 履歴をみて候補の重みを計算する */
static int
get_history_weight(xstr *xs)
{
  int i, nr = anthy_get_nr_values();
  int w = 0;
  for (i = 0; i < nr; i++) {
    xstr *h = anthy_get_nth_xstr(i);
    if (!h) {
      continue;
    }
    if (!anthy_xstrcmp(xs, h)) {
      w++;
      if (i == 0) {
	/* 直前に確定されたものには高いスコア*/
	w += (HISTORY_DEPTH / 2);
      }
    }
  }
  return w;
}

/* 履歴で加点する */
void
anthy_reorder_candidates_by_history(struct seg_ent *se)
{
  int i, primary_score;
  /**/
  if (anthy_select_section("CAND_HISTORY", 1)) {
    return ;
  }
  if (anthy_select_row(&se->str, 0)) {
    return ;
  }
  /* 最も評価の高い候補 */
  primary_score = se->cands[0]->score;
  /**/
  for (i = 0; i < se->nr_cands; i++) {
    struct cand_ent *ce = se->cands[i];
    int weight = get_history_weight(&ce->str);
    ce->score += primary_score / (HISTORY_DEPTH /2) * weight;
  }
  anthy_mark_row_used();
}

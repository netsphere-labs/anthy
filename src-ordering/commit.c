/*
 * 確定(コミット)後の処理をする。
 * 各種の学習処理を呼び出す
 *
 * anthy_proc_commit() が外部から呼ばれる
 */
#include <stdlib.h>
#include <time.h>

#include <ordering.h>
#include <record.h>
#include "splitter.h"
#include "segment.h"
#include "sorter.h"

#define MAX_OCHAIRE_ENTRY_COUNT 100
#define MAX_OCHAIRE_LEN 32
#define MAX_PREDICTION_ENTRY 100

/* 交換された候補を探す */
static void
learn_swapped_candidates(struct segment_list *sl)
{
  int i;
  struct seg_ent *seg;
  for (i = 0; i < sl->nr_segments; i++) {
    seg = anthy_get_nth_segment(sl, i);
    if (seg->committed != 0) {
      /* 最初の候補(0番目)でない候補(seg->commited番目)がコミットされた */
      anthy_swap_cand_ent(seg->cands[0],
			  seg->cands[seg->committed]);
    }
  }
  anthy_cand_swap_ageup();
}

/* 長さが変わった文節に対して */
static void
learn_resized_segment(struct splitter_context *sc,
		      struct segment_list *sl)
		      
{
  int i;
  struct meta_word **mw
    = alloca(sizeof(struct meta_word*) * sl->nr_segments);
  int *len_array
    = alloca(sizeof(int) * sl->nr_segments);

  /* 各文節の長さの配列とmeta_wordの配列を用意する */
  for (i = 0; i < sl->nr_segments; i++) {
    struct seg_ent *se = anthy_get_nth_segment(sl, i);
    mw[i] = se->cands[se->committed]->mw;
    len_array[i] = se->str.len;
  }

  anthy_commit_border(sc, sl->nr_segments, mw, len_array);
}

/* recordにお茶入れ学習の結果を書き込む */
static void
commit_ochaire(struct seg_ent *seg, int count, xstr* xs)
{
  int i;
  if (xs->len >= MAX_OCHAIRE_LEN) {
    return ;
  }
  if (anthy_select_row(xs, 1)) {
    return ;
  }
  anthy_set_nth_value(0, count);
  for (i = 0; i < count; i++, seg = seg->next) {
    anthy_set_nth_value(i * 2 + 1, seg->len);
    anthy_set_nth_xstr(i * 2 + 2, &seg->cands[seg->committed]->str);
  }
}

/* recordの領域を節約するために、お茶入れ学習のネガティブな
   エントリを消す */
static void
release_negative_ochaire(struct splitter_context *sc,
			 struct segment_list *sl)
{
  int start, len;
  xstr xs;
  (void)sl;
  /* 変換前のひらがな文字列 */
  xs.len = sc->char_count;
  xs.str = sc->ce[0].c;

  /* xsの部分文字列に対して */
  for (start = 0; start < xs.len; start ++) {
    for (len = 1; len <= xs.len - start && len < MAX_OCHAIRE_LEN; len ++) {
      xstr part;
      part.str = &xs.str[start];
      part.len = len;
      if (anthy_select_row(&part, 0) == 0) {
	anthy_release_row();
      }
    }
  }
}

/* お茶入れ学習を行う */
static void
learn_ochaire(struct splitter_context *sc,
	      struct segment_list *sl)
{
  int i;
  int count;

  if (anthy_select_section("OCHAIRE", 1)) {
    return ;
  }

  /* お茶入れ学習のネガティブなエントリを消す */
  release_negative_ochaire(sc, sl);

  /* お茶入れ学習をする */
  for (count = 2; count <= sl->nr_segments && count < 5; count++) {
    /* 2文節以上の長さの文節列に対して */

    for (i = 0; i <= sl->nr_segments - count; i++) {
      struct seg_ent *head = anthy_get_nth_segment(sl, i);
      struct seg_ent *s;
      xstr xs;
      int j;
      xs = head->str;
      if (xs.len < 2 && count < 3) {
	/* 細切れの文節を学習することを避ける、
	 * いい加減なheuristics */
	continue;
      }
      /* 文節列を構成する文字列を作る */
      for (j = 1, s = head->next; j < count; j++, s = s->next) {
	xs.len += s->str.len;
      }
      /**/
      commit_ochaire(head, count, &xs);
    }
  }
  if (anthy_select_section("OCHAIRE", 1)) {
    return ;
  }
  anthy_truncate_section(MAX_OCHAIRE_ENTRY_COUNT);
}

static void
learn_prediction(struct segment_list *sl)
{
  int i, j;
  int added = 0;
  if (anthy_select_section("PREDICTION", 1)) {
    return ;
  }
  for (i = 0; i < sl->nr_segments; i++) {
    struct seg_ent *seg = anthy_get_nth_segment(sl, i);
    int nr_predictions;
    time_t t = time(NULL);
    xstr *xs = &seg->cands[seg->committed]->str;

    if (seg->committed < 0) {
      continue;
    }
    if (anthy_select_row(&seg->str, 1)) {
      continue;
    }
    nr_predictions = anthy_get_nr_values();

    /* 既に履歴にある場合はタイムスタンプだけ更新 */
    for (j = 0; j < nr_predictions; j += 2) {
      xstr *log = anthy_get_nth_xstr(j + 1);
      if (!log) {
	continue;
      }
      if (anthy_xstrcmp(log, xs) == 0) {
	anthy_set_nth_value(j, t);
	break;
      }
    }
    /* ない場合は末尾に追加 */
    if (j == nr_predictions) {
      anthy_set_nth_value(nr_predictions, t);
      anthy_set_nth_xstr(nr_predictions + 1, xs);      
      added = 1;
    }
  }
  if (added) {
    anthy_truncate_section(MAX_PREDICTION_ENTRY);
  }
}

void
anthy_proc_commit(struct segment_list *sl,
		  struct splitter_context *sc)
{
  /* 各種の学習を行う */
  learn_swapped_candidates(sl);
  learn_resized_segment(sc, sl);
  learn_ochaire(sc, sl);
  learn_prediction(sl);
  anthy_learn_cand_history(sl);
}

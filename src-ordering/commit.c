/*
 * 確定(コミット)後の処理をする。
 * 各種の学習処理を呼び出す
 *
 * anthy_proc_commit() が外部から呼ばれる
 */
#include <stdlib.h>

#include <ordering.h>
#include <record.h>
#include "splitter.h"
#include "segment.h"
#include "sorter.h"

#define MAX_OCHAIRE_ENTRY_COUNT 100
#define MAX_OCHAIRE_LEN 32

/* 交換された候補を探す */
static void
learn_swapped_candidates(struct segment_list *sl)
{
  int i;
  struct seg_ent *seg;
  for (i = 0; i < sl->nr_segments; i++) {
    seg = anthy_get_nth_segment(sl, i);
    if (seg->committed > 0) {
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
  if (anthy_select_column(xs, 1)) {
    return ;
  }
  anthy_set_nth_value(0, count);
  for (i = 0; i < count; i++, seg = seg->next) {
    anthy_set_nth_value(i * 2 + 1, seg->len);
    anthy_set_nth_xstr(i * 2 + 2, &seg->cands[seg->committed]->str);
  }
}

/* お茶入れ学習のネガティブなエントリを消す */
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

  /* xsの全ての部分文字列に対して */
  for (start = 0; start < xs.len; start ++) {
    for (len = 1; len <= xs.len - start && len < MAX_OCHAIRE_LEN; len ++) {
      xstr part;
      part.str = &xs.str[start];
      part.len = len;
      if (anthy_select_column(&part, 0) == 0) {
	anthy_release_column();
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
      for (j = 1, s = head->next; j < count; j++, s = s->next) {
	xs.len += s->str.len;
      }
      commit_ochaire(head, count, &xs);
    }
  }
  if (anthy_select_section("OCHAIRE", 1)) {
    return ;
  }
  anthy_truncate_section(MAX_OCHAIRE_ENTRY_COUNT);
}

static int
check_segment_relation(struct seg_ent *cur, struct seg_ent *target)
{
  /* 先頭の候補で確定されたので、学習しない */
  if (cur->committed == 0) {
    return 0;
  }
  /* 単純な形式の文節しか学習しない */
  if (cur->cands[0]->nr_words != 1 ||
      cur->cands[cur->committed]->nr_words != 1 ||
      target->cands[target->committed]->nr_words != 1) {
    return 0;
  }
  /* 確定された文節の品詞と、最初に出した候補の品詞が同じであることを確認 */
  if (anthy_wtype_get_pos(cur->cands[0]->elm[0].wt) !=
      anthy_wtype_get_pos(cur->cands[cur->committed]->elm[0].wt)) {
    return 0;
  }
  if (cur->cands[cur->committed]->elm[0].id == -1 ||
      target->cands[target->committed]->elm[0].id == -1) {
    return 0;
  }
  /* 辞書に対して登録をする */
  anthy_dic_register_relation(target->cands[target->committed]->elm[0].id,
			      cur->cands[cur->committed]->elm[0].id);
  return 1;
			      
}

static void
learn_word_relation(struct segment_list *sl)
{
  int i, j;
  int nr_learned = 0;
  for (i = 0; i < sl->nr_segments; i++) {
    struct seg_ent *cur = anthy_get_nth_segment(sl, i);
    for (j = i - 2; j < i + 2 && j < sl->nr_segments; j++) {
      struct seg_ent *target;
      if (i == j || j < 0) {
	continue;
      }
      target = anthy_get_nth_segment(sl, j);
      /* i番目とj番目の文節の候補の関係を学習できるかチェックする */
      nr_learned += check_segment_relation(cur, target);
    }
  }
  if (nr_learned) {
    anthy_dic_commit_relation();
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
  learn_word_relation(sl);
  anthy_learn_cand_history(sl);
}

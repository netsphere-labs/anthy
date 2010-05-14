/* features
 *
 * 素性の番号と意味を隠蔽して管理する
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "segclass.h"
#include "feature_set.h"
/* for MW_FEATURE* constants */
#include "splitter.h"

/* 素性の番号 
 *
 * 0-19 クラス素性
 * 30-319(30+SEG_SIZE^2) クラス遷移属性
 * 540-579 その他
 * 580- (512個) 付属語の種類
 */

#define CUR_CLASS_BASE 0
#define DEP_TYPE_FEATURE_BASE 20
#define CLASS_TRANS_BASE 30
#define FEATURE_SV 542
#define FEATURE_WEAK 543
#define FEATURE_SUFFIX 544
#define FEATURE_PREV_WEAK 545
#define FEATURE_NUM 546
#define FEATURE_CORE1 547
#define FEATURE_HIGH_FREQ 548
#define DEP_FEATURE_BASE 580

void
anthy_feature_list_init(struct feature_list *fl)
{
  fl->nr = 0;
  fl->size = NR_EM_FEATURES;
}

void
anthy_feature_list_free(struct feature_list *fl)
{
  (void)fl;
}

void
anthy_feature_list_add(struct feature_list *fl, int f)
{
  if (fl->nr < NR_EM_FEATURES) {
    fl->u.index[fl->nr] = f;
    fl->nr++;
  }
}

int
anthy_feature_list_nr(const struct feature_list *fl)
{
  return fl->nr;
}

int
anthy_feature_list_nth(const struct feature_list *fl, int nth)
{
  return fl->u.index[nth];
}

static int
cmp_short(const void *p1, const void *p2)
{
  return *((short *)p1) - *((short *)p2);
}

void
anthy_feature_list_sort(struct feature_list *fl)
{
  qsort(fl->u.index, fl->nr, sizeof(fl->u.index[0]),
	cmp_short);
}


void
anthy_feature_list_set_cur_class(struct feature_list *fl, int cl)
{
  anthy_feature_list_add(fl, CUR_CLASS_BASE + cl);
}

void
anthy_feature_list_set_class_trans(struct feature_list *fl, int pc, int cc)
{
  anthy_feature_list_add(fl, CLASS_TRANS_BASE + pc * SEG_SIZE + cc);
}

void
anthy_feature_list_set_dep_word(struct feature_list *fl, int h)
{
  anthy_feature_list_add(fl, h + DEP_FEATURE_BASE);
}

void
anthy_feature_list_set_dep_class(struct feature_list *fl, int c)
{
  anthy_feature_list_add(fl, c + DEP_TYPE_FEATURE_BASE);
}

void
anthy_feature_list_set_prev_weak(struct feature_list *fl)
{
  anthy_feature_list_add(fl, FEATURE_PREV_WEAK);
}

void
anthy_feature_list_set_mw_features(struct feature_list *fl, int mask)
{
  if (mask & MW_FEATURE_WEAK) {
    anthy_feature_list_add(fl, FEATURE_WEAK);
  }
  if (mask & MW_FEATURE_SUFFIX) {
    anthy_feature_list_add(fl, FEATURE_SUFFIX);
  }
  if (mask & MW_FEATURE_SV) {
    anthy_feature_list_add(fl, FEATURE_SV);
  }
  if (mask & MW_FEATURE_NUM) {
    anthy_feature_list_add(fl, FEATURE_NUM);
  }
  if (mask & MW_FEATURE_CORE1) {
    anthy_feature_list_add(fl, FEATURE_CORE1);
  }
  if (mask & MW_FEATURE_HIGH_FREQ) {
    anthy_feature_list_add(fl, FEATURE_HIGH_FREQ);
  }
}

void
anthy_feature_list_print(struct feature_list *fl)
{
  int i;
  printf("features=");
  for (i = 0; i < fl->nr; i++) {
    if (i) {
      printf(",");
    }
    printf("%d", fl->u.index[i]);
  }
  printf("\n");
}

static int
compare_line(const void *kp, const void *cp)
{
  const int *f = kp;
  const struct feature_freq *c = cp;
  int i;
  for (i = 0; i < NR_EM_FEATURES; i++) {
    if (f[i] != c->f[i]) {
      return f[i] - c->f[i];
    }
  }
  return 0;
}

struct feature_freq *
anthy_find_feature_freq(const struct feature_list *fl,
			const struct feature_freq *array,
			int nr_lines)
{
  int i, nr;
  struct feature_freq *res;
  int f[10];
  /* 配列にコピーする */
  nr = anthy_feature_list_nr(fl);
  for (i = 0; i < 10; i++) {
    if (i < nr) {
      f[i] = anthy_feature_list_nth(fl, i);
    } else {
      f[i] = 0;
    }
  }
  res = bsearch(f, array, nr_lines,
		sizeof(struct feature_freq),
		compare_line);
  return res;
}

void
anthy_init_features(void)
{
}

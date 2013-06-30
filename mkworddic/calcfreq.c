/* 単語の頻度を計算する */
#include <stdlib.h>
#include "mkdic.h"

static int
count_nr_words(struct yomi_entry_list *yl)
{
  int nr = 0;
  struct yomi_entry *ye;
  for (ye = yl->head; ye; ye = ye->next) {
    nr += ye->nr_entries;
  }
  return nr;
}

static struct word_entry **
make_word_array(struct yomi_entry_list *yl, int nr)
{
  struct word_entry **array = malloc(sizeof(struct word_entry *) *
				    nr);
  int nth = 0;
  struct yomi_entry *ye;
  for (ye = yl->head; ye; ye = ye->next) {
    int i;
    for (i = 0; i < ye->nr_entries; i++) {
      array[nth] = &ye->entries[i];
      nth ++;
    }
  }
  return array;
}

/** qsort用の比較関数 */
static int
compare_word_entry_by_freq(const void *p1, const void *p2)
{
  const struct word_entry * const *e1 = p1;
  const struct word_entry * const *e2 = p2;
  return abs((*e2)->raw_freq) - abs((*e1)->raw_freq);
}

static void
set_freq(struct word_entry **array, int nr)
{
  int i;
  int percent = nr / 100;
  for (i = 0; i < nr; i++) {
    struct word_entry *we = array[i];
    we->freq = 99 - (i / percent);
    if (we->freq < 1) {
      we->freq = 1;
    }
    we->freq *= 100;
    we->freq -= we->source_order;
    if (we->raw_freq < 0) {
      we->freq *= -1;
    }
  }
}

void
calc_freq(struct yomi_entry_list *yl)
{
  int nr;
  struct word_entry **we;
  /**/
  nr = count_nr_words(yl);
  we = make_word_array(yl, nr);
  /**/
  qsort(we, nr,
	sizeof(struct word_entry *),
	compare_word_entry_by_freq);
  set_freq(we, nr);
  /**/
  free(we);
}


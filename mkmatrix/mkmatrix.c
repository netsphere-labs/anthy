/*
 * 接続行列を生成する
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wtype.h"
#include "diclib.h"
#include "matrix.h"

#define LINE_LEN 1024

#define ELM_NONE 0
#define ELM_INDEP 1
#define ELM_DEP 2

/* 文中の自立語もしくは付属語部 */
struct elm_word {
  /* ELM_* */
  int type;
  /**/
  int id, yomi_hash;
  char wt[16];
  /* common */
  int hash;
};

struct sentence {
  int nr_words;
  struct elm_word **words;
  /**/
  struct sentence *next;
};

static struct sentence *
sentence_new(void)
{
  struct sentence *st = malloc(sizeof(*st));
  st->nr_words = 0;
  st->words = NULL;
  return st;
}

static struct elm_word *
elm_word_new(void)
{
  return malloc(sizeof(struct elm_word));
}

static void
sentence_push_back_word_elm(struct sentence *st,
			    struct elm_word *elm)
{
  st->words = realloc(st->words,
		      sizeof(*elm) * (st->nr_words + 1));
  st->words[st->nr_words] = elm;
  st->nr_words ++;
}

static void
sentence_push_back_indep(struct sentence *st, char *str)
{
  struct elm_word *w = elm_word_new();
  w->type = ELM_INDEP;
  sscanf(str, "id=%d hash=%d yomi_hash=%d %s",
	 &w->id, &w->hash, &w->yomi_hash, w->wt);
  sentence_push_back_word_elm(st, w);
}

static void
sentence_push_back_dep(struct sentence *st, char *str)
{
  struct elm_word *w = elm_word_new();
  w->type = ELM_DEP;
  sscanf(str, "hash=%d", &w->hash);
  sentence_push_back_word_elm(st, w);
}

static void
append(struct sentence *lst, struct sentence *st)
{
  struct elm_word *w;
  if (!st) {
    return ;
  }
  w = elm_word_new();
  w->type = ELM_NONE;
  sentence_push_back_word_elm(st, w);  
  st->next = lst->next;
  lst->next = st;
}

static void
parse(struct sentence *lst, FILE *fp)
{
  char buf[LINE_LEN];
  struct sentence *st = NULL;
  while (fgets(buf, LINE_LEN, fp)) {
    if (!strncmp(buf, "segments:", 9)) {
      if (st) {
	append(lst, st);
      }
      st = sentence_new();
    } else if (!strncmp(buf, "indep_word ", 11)) {
      sentence_push_back_indep(st, &buf[11]);
    } else if (!strncmp(buf, "dep_word ", 9)) {
      sentence_push_back_dep(st, &buf[9]);
    }
  }
  if (st) {
    append(lst, st);
  }
}

static int
wt_name_to_id(const char *name)
{
  wtype_t wt;
  int pos, cc, cosp;
  if (!anthy_type_to_wtype(name, &wt)) {
    printf("unknown type (%s)\n", name);
    return 0;
  }
  pos = anthy_wtype_get_pos(wt);
  cc = anthy_wtype_get_cc(wt);
  cosp = anthy_wtype_get_cos(wt);
  if (cc == CC_NONE) {
    /* 活用しない単語の場合、副品詞(COS)を使う */
    return pos * 100 + cosp;
  }
  /* 活用する場合、活用型(CC)を使う */
  return pos * 100 + cc;
}

static void
mark_matrix(struct sparse_matrix *m, int row, int col)
{
  int cur = anthy_sparse_matrix_get_int(m, row, col);
  cur ++;
  anthy_sparse_matrix_set(m, row, col, cur, NULL);
}

static void
build(struct sparse_matrix *m, struct sentence *lst)
{
  struct sentence *st;
  for (st = lst->next; st; st = st->next) {
    struct elm_word *w1, *w2;
    int i;
    for (i = 0; i < st->nr_words - 1; i++) {
      w1 = st->words[i];
      w2 = st->words[i+1];
      /* 自立語、付属語のパターン */
      if (w1->type == ELM_INDEP &&
	  w2->type == ELM_DEP) {
	int id = wt_name_to_id(w1->wt);
	/*printf("%s %d %d\n", w1->wt, id, w2->hash);*/
	mark_matrix(m, id, w2->hash);
      }
      if (w1->type == ELM_INDEP &&
	  w2->type == ELM_INDEP) {
	/* 自立語、自立語のパターン */
	int id = wt_name_to_id(w1->wt);
	mark_matrix(m, id, 0);
      }
    }
  }
  anthy_sparse_matrix_make_matrix(m);
}

static void
write_out(struct sparse_matrix *m, const char *ofn)
{
  struct matrix_image *mi;
  int i;
  FILE *fp;

  /**/
  fp = NULL;
  if (ofn) {
    fp = fopen(ofn, "w");
  }

  /**/
  mi = anthy_matrix_image_new(m);
  for (i = 0; i < mi->size; i++) {
    /*printf("%d %x\n", i, mi->image[i]);*/
    if (fp) {
      int v = anthy_dic_htonl(mi->image[i]);
      fwrite(&v, sizeof(int), 1, fp);
    }
  }

  /**/
  if (fp) {
    fclose(fp);
  }
}


int
main(int argc, char **argv)
{
  struct sparse_matrix *m;
  struct sentence st;
  int i;
  const char *prev_arg = "";
  const char *ofn = NULL;

  /**/
  m = anthy_sparse_matrix_new();
  st.next = NULL;

  /**/
  for (i = 1; i < argc; i++) {
    char *arg = argv[i];
    FILE *fp;

    if (!strcmp(prev_arg, "-o")) {
      ofn = arg;
    } else if (arg[0] != '-') {
      fp = fopen(arg, "r");
      if (fp) {
	parse(&st, fp);
	fclose(fp);
      }
    }
    /**/
    prev_arg = arg;
  }
  build(m, &st);
  write_out(m, ofn);
  return 0;
}

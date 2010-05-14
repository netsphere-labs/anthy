/*
 * 文節の遷移行列を作成する
 *
 * morphological-analyzerの出力には下記のマークが付けてある
 * ~ 候補の誤り
 * ! 文節長の誤り
 * ^ 複合文節の2つめ以降の要素
 *
 * generate transition matrix
 *
 * Copyright (C) 2006 HANAOKA Toshiyuki
 * Copyright (C) 2006-2007 TABATA Yusuke
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../include/feature_set.h"
#include "input_set.h"

#define MAX_FEATURE 1100
#define FEATURE_SET_SIZE NR_EM_FEATURES

/* 文節の連接行列 */
struct matrix {
  /**/
  struct input_set *cand_is;
  struct input_set *seg_is;
  /**/
  int nr_sentences;
  int nr_connections;
};

struct array {
  int len;
  int f[16];
};

static int dbg_flag;

static struct matrix *
init_matrix(void)
{
  struct matrix *m;
  m = malloc(sizeof(struct matrix));
  m->seg_is = input_set_create(MAX_FEATURE);
  m->cand_is = input_set_create(MAX_FEATURE);
  m->nr_sentences = 0;
  m->nr_connections = 0;
  return m;
}

static void
parse_features(struct array *features, char *s)
{
  char *tok, *str = s;
  tok = strtok(str, ",");
  features->len = 0;
  do {
    features->f[features->len] = atoi(tok);
    features->len++;
    tok = strtok(NULL, ",");
  } while(tok);
}

static void
add_seg_struct_info(struct matrix *m,
		    struct array *features,
		    double weight)
{
  input_set_set_features(m->cand_is, features->f, features->len, weight);
}

static void
read_morph_file(struct matrix *m, FILE *fp)
{
  char line[1024];
  struct array features;
  double weight = 1.0;

  while (fgets(line, 1024, fp)) {
    char *buf = line;
    int error_class = 0;
    if (!strncmp(buf, "eos", 3)) {
      m->nr_sentences ++;
    }
    if (line[0] == '~' || line[0] == '!' ||
	line[0] == '^') {
      buf ++;
      error_class = 1;
    }
    if (!strncmp(buf, "indep_word", 10) ||
	!strncmp(buf, "eos", 3)) {
      char *s;
      /**/
      s = strstr(buf, "features=");
      if (s) {
	s += 9;
	parse_features(&features, s);
	m->nr_connections ++;
      }
      if (error_class) {
	if (line[0] == '~') {
	  add_seg_struct_info(m, &features, -weight);
	}
	if (line[0] == '!') {
	  input_set_set_features(m->seg_is, features.f, features.len, -weight);
	}
      } else {
	/* 接続行列 */
	input_set_set_features(m->seg_is, features.f, features.len, weight);
	/* 文節の構造 */
	add_seg_struct_info(m, &features, weight);
      }
    }
  }
}

static void
read_file(struct matrix *m, char *fn)
{
  FILE *ifp;
  ifp = fopen(fn, "r");
  if (!ifp) {
    return ;
  }
  read_morph_file(m, ifp);
  fclose(ifp);
}

static void
dump_line(FILE *ofp, struct input_line *il)
{
  int i;
  fprintf(ofp, "{{");
  for (i = 0; i < FEATURE_SET_SIZE || i < il->nr_features; i++) {
    if (i) {
      fprintf(ofp, ", ");
    }
    if (i < il->nr_features) {
      fprintf(ofp, "%d", il->features[i]);
    } else {
      fprintf(ofp, "0");
    }
  }
  fprintf(ofp,",%d,%d", (int)il->negative_weight, (int)il->weight);
  fprintf(ofp, "}},\n");
}

static int
compare_line(const void *p1, const void *p2)
{
  const struct input_line *const *il1 = p1;
  const struct input_line *const *il2 = p2;
  int i;
  for (i = 0; i < (*il1)->nr_features &&
	 i < (*il2)->nr_features; i++) {
    if ((*il1)->features[i] !=
	(*il2)->features[i]) {
      return (*il1)->features[i] - (*il2)->features[i];
    }
  }
  return (*il1)->nr_features - (*il2)->nr_features;
}

static void
dump_cand_features(FILE *ofp, struct input_set *is)
{
  struct input_line *il, **lines;
  int i, nr = 0;
  int weight = 0;

  /* count lines */
  for (il = input_set_get_input_line(is); il; il = il->next_line) {
    nr ++;
    weight += (int)il->weight;
  }
  /* copy lines */
  lines = malloc(sizeof(struct input_line *) * nr);
  for (il = input_set_get_input_line(is), i = 0; i < nr; i++, il = il->next_line) {
    lines[i] = il;
  }
  /* sort */
  qsort(lines, nr, sizeof(struct input_line *), compare_line);
  /* output */
  fprintf(ofp, "static const int total_line_weight = %d;\n", weight);
  fprintf(ofp, "static const int total_line_count = %d;\n", nr);
  fprintf(ofp, "static const struct feature_freq feature_array[] = {\n");
  for (i = 0; i < nr; i++) {
    dump_line(ofp, lines[i]);
  }
  fprintf(ofp, "};\n");
}

static void
proc_corpus(int nr_fn, char **fns, FILE *ofp)
{
  int i;
  struct matrix *m;
  /**/
  m = init_matrix();
  for (i = 0; i < nr_fn; i++) {
    read_file(m, fns[i]);
  }

  /* segment transition information */
  fprintf(ofp, "#ifdef TRANSITION_INFO\n");
  dump_cand_features(ofp, m->seg_is);
  fprintf(ofp, "#endif\n");
  /* candidate ordering information */
  fprintf(ofp, "#ifdef CAND_INFO\n");
  dump_cand_features(ofp, m->cand_is);
  fprintf(ofp, "#endif\n");

  /**/
  fprintf(stderr, " %d sentences\n", m->nr_sentences);
  fprintf(stderr, " %d connections\n", m->nr_connections);
  fprintf(stderr, " %d segments\n", m->nr_connections - m->nr_sentences);
}

int
main(int argc, char **argv)
{
  FILE *ofp;
  int i;
  int nr_input = 0;
  char **input_files;

  ofp = NULL;
  input_files = malloc(sizeof(char *) * argc);
  
  for (i = 1; i < argc; i++) {
    char *arg = argv[i];
    if (!strcmp(arg, "-o")) {
      ofp = fopen(argv[i+1], "w");
      i ++;
    } else if (!strcmp(arg, "-d")) {
      dbg_flag = 1;
    } else {
      input_files[nr_input] = arg;
      nr_input ++;
    }
  }
  if (!ofp) {
    ofp = stdout;
  }
  proc_corpus(nr_input, input_files, ofp);

  return 0;
}

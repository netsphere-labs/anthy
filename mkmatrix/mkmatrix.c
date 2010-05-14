/*
 * 接続行列を生成する
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xstr.h"
#include "wtype.h"
#include "diclib.h"
#include "matrix.h"

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

static double
calc_rate(int count)
{
  return 100 * log(count + 1);
}

static void
matrix_set(struct sparse_matrix *m, int hash, int row, int count)
{
  double rate;
  if (count > 1) {
    rate = calc_rate(count);
    anthy_sparse_matrix_set(m, row, hash, (int)rate, NULL);
  }
}

static void
parse(FILE *fp, struct sparse_matrix *m)
{
  char line[256];
  int ln = 0;
  while (fgets(line, 256, fp)) {
    char buf[256];
    int count, hash;
    int noun, noun_35, noun_30;
    int verb, other;
    xstr *xs;
    if (sscanf(line, "%d %d %d %d %d %s",
	       &count, &noun, &noun_35, &noun_30,
	       &verb, buf) != 6) {
      continue;
    }
    other = count - noun - verb;
    ln ++;
    /*printf("%d %s\n", ln, buf);*/
    xs = anthy_cstr_to_xstr(buf, 0);
    hash = anthy_xstr_hash(xs);
    anthy_free_xstr(xs);
    /**/
    matrix_set(m, hash, 0, count);
    matrix_set(m, hash, 1, noun);
    matrix_set(m, hash, 2, verb);
    matrix_set(m, hash, 3, other);
    matrix_set(m, hash, 4, noun_35);
    matrix_set(m, hash, 5, noun_30);
  }
}

int
main(int argc, char **argv)
{
  struct sparse_matrix *m;
  int i;
  const char *prev_arg = "";
  const char *ofn = "matrix";

  /**/
  m = anthy_sparse_matrix_new();

  /**/
  for (i = 1; i < argc; i++) {
    char *arg = argv[i];
    FILE *fp;

    if (!strcmp(prev_arg, "-o")) {
      ofn = arg;
    } else if (arg[0] != '-') {
      fp = fopen(arg, "r");
      if (fp) {
	parse(fp, m);
	fclose(fp);
      }
    }
    /**/
    prev_arg = arg;
  }
  anthy_sparse_matrix_make_matrix(m);
  write_out(m, ofn);
  return 0;
}

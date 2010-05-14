/* 入力のセットを管理するコード
 * TODO: IIS関連を消す
 *
 * Copyright (C) 2006 HANAOKA Toshiyuki
 *
 * Special Thanks: Google Summer of Code Program 2006
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "input_set.h"

struct input_set {
  int max_feature;
  double *feature_weight;
  double *negative_weight;
  /**/
  double total_weight;
  struct input_line *lines;
};

/* too slow */
static struct input_line *
find_same_line(struct input_set *is, int *features, int nr)
{
  struct input_line *il;
  for (il = is->lines; il; il = il->next_line) {
    int i;
    if (il->nr_features != nr) {
      continue;
    }
    for (i = 0; i < nr; i++) {
      if (il->features[i] != features[i]) {
	break;
      }
    }
    if (i >= nr) {
      return il;
    }
  }
  return NULL;
}

static struct input_line *
add_line(struct input_set *is, int *features, int nr)
{
  int i;
  struct input_line *il;
  il = malloc(sizeof(struct input_line));
  il->nr_features = nr;
  il->features = malloc(sizeof(double) * nr);
  for (i = 0; i < nr; i++) {
    il->features[i] = features[i];
  }
  il->weight = 0;
  il->negative_weight = 0;
  /* link */
  il->next_line = is->lines;
  is->lines = il;
  return il;
}

static void
accumlate_features(struct input_set *is, int *features,
		   int nr, double weight)
{
  int i;
  for (i = 0; i < nr; i++) {
    int f = features[i];
    is->feature_weight[f] += weight;
  }
}

static void
accumlate_negative_features(struct input_set *is, int *features,
			    int nr, double weight)
{
  int i;
  for (i = 0; i < nr; i++) {
    int f = features[i];
    is->negative_weight[f] -= weight;
  }
}

void
input_set_set_features(struct input_set *is, int *features,
		       int nr, double weight)
{
  struct input_line *il;
  double abs_weight = fabs(weight);

  if (weight < 0) {
    accumlate_negative_features(is, features, nr, weight);
  } else {
    accumlate_features(is, features, nr, weight);
  }

  /**/
  il = find_same_line(is, features, nr);
  if (!il) {
    il = add_line(is, features, nr);
  }
  /**/
  if (weight > 0) {
    il->weight += weight;
  } else {
    il->negative_weight += abs_weight;
  }
  is->total_weight += abs_weight;
}

void
iis_dump(struct input_set *is)
{
  struct input_line *il;
  int i;
  printf("total_weight,%f\n", is->total_weight);
  for (il = is->lines; il; il = il->next_line) {
    printf(" nr=%f ", il->weight);
    for (i = 0; i < il->nr_features; i++) {
      printf("%d,", il->features[i]);
    }
    printf(" (%f)\n", il->weight / is->total_weight);
  }
}

struct input_set *
input_set_create(int nr)
{
  struct input_set *is;
  int i;
  is = malloc(sizeof(struct input_set));
  is->lines = NULL;
  is->total_weight = 0;
  /**/
  is->max_feature = nr;
  is->feature_weight = malloc(sizeof(double) * nr);
  is->negative_weight = malloc(sizeof(double) * nr);
  for (i = 0; i < nr; i++) {
    is->feature_weight[i] = 0;
    is->negative_weight[i] = 0;
  }
  return is;
}

struct input_line *
input_set_get_input_line(struct input_set *is)
{
  return is->lines;
}

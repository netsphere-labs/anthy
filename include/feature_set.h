/* 素性を扱う */
#ifndef _feature_set_h_included_
#define _feature_set_h_included_

#define NR_EM_FEATURES 8

struct feature_list {
  /* いまのところ、素性は8個まで */
  int nr;
  int size;
  union {
    short index[NR_EM_FEATURES];
    short *array;
  } u;
};

struct feature_freq {
  const int f[NR_EM_FEATURES + 2];
};

void anthy_init_features(void);
struct feature_freq *
anthy_find_feature_freq(const struct feature_list *fl,
			const struct feature_freq *array,
			int nr_lines);

/**/
void anthy_feature_list_init(struct feature_list *fl);
void anthy_feature_list_free(struct feature_list *fl);
/**/
void anthy_feature_list_add(struct feature_list *fl, int f);
int anthy_feature_list_nr(const struct feature_list *fl);
int anthy_feature_list_nth(const struct feature_list *fl, int nth);
void anthy_feature_list_sort(struct feature_list *fl);
/**/
void anthy_feature_list_set_cur_class(struct feature_list *fl, int cc);
void anthy_feature_list_set_class_trans(struct feature_list *fl, int pc, int cc);
void anthy_feature_list_set_dep_word(struct feature_list *fl, int h);
void anthy_feature_list_set_prev_weak(struct feature_list *fl);
void anthy_feature_list_set_dep_class(struct feature_list *fl, int c);
void anthy_feature_list_set_mw_features(struct feature_list *fl, int mask);
/**/
void anthy_feature_list_print(struct feature_list *fl);

#endif

#ifndef _input_set_h_included_
#define _input_set_h_included_

struct input_set;

struct input_line {
  /**/
  double weight;
  double negative_weight;
  /**/
  int nr_features;
  int *features;
  /**/
  struct input_line *next_line;
};

struct input_set *input_set_create(int nr_features);
void input_set_set_features(struct input_set *is, int *features,
			    int nr, double strength);
/**/
void iis_dump(struct input_set *is);

/**/
struct input_line *input_set_get_input_line(struct input_set *is);

#endif

/* リリース前のチェックを行う */
#include <stdio.h>
#include <stdlib.h>
#include <anthy/anthy.h>
#include <anthy/xstr.h>

static int
init(void)
{
  int res;

  res = anthy_init();
  if (res) {
    printf("failed to init\n");
    return 1;
  }
  anthy_quit();
  /* init again */
  res = anthy_init();
  if (res) {
    printf("failed to init\n");
    return 1;
  }
  return 0;
}

static int
test0(void)
{
  anthy_context_t ac;
  ac = anthy_create_context();
  if (!ac) {
    printf("failed to create context\n");
    return 1;
  }
  anthy_release_context(ac);
  return 0;
}

static int
test1(void)
{
  anthy_context_t ac;
  char buf[100];
  xstr *xs;
  ac = anthy_create_context();
  if (!ac) {
    printf("failed to create context\n");
    return 1;
  }
  anthy_set_string(ac, "あいうえお、かきくけこ。");
  if (anthy_get_segment(ac, 0, NTH_UNCONVERTED_CANDIDATE, buf, 100) > 0) {
    printf("(%s)\n", buf);
  }
  if (anthy_get_segment(ac, 0, NTH_KATAKANA_CANDIDATE, buf, 100) > 0) {
    printf("(%s)\n", buf);
  }
  if (anthy_get_segment(ac, 0, NTH_HIRAGANA_CANDIDATE, buf, 100) > 0) {
    printf("(%s)\n", buf);
  }
  if (anthy_get_segment(ac, 0, NTH_HALFKANA_CANDIDATE, buf, 100) > 0) {
    printf("(%s)\n", buf);
  }
  anthy_release_context(ac);
  xs = anthy_cstr_to_xstr("あいうえおがぎぐげご", 0);
  xs = anthy_xstr_hira_to_half_kata(xs);
  anthy_putxstrln(xs);
  return 0;
}

static int
shake_test(const char *str)
{
  int i;
  anthy_context_t ac;
  ac = anthy_create_context();
  if (!ac) {
    printf("failed to create context\n");
    return 1;
  }
  anthy_set_string(ac, str);
  for (i = 0; i < 50; i++) {
    int res, nth, rsz;
    struct anthy_conv_stat cs;
    res = anthy_get_stat(ac, &cs);
    nth = rand() % cs.nr_segment;
    rsz = (rand() % 3) - 1;
    anthy_resize_segment(ac, nth, rsz);
  }
  anthy_release_context(ac);
  return 0;
}

int
main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  printf("checking\n");
  if (init()) {
    printf("fail (init)\n");
    return 0;
  }
  if (test0()) {
    printf("fail (test0)\n");
  }
  if (test1()) {
    printf("fail (test1)\n");
  }
  if (shake_test("あいうえおかきくけこ")) {
    printf("fail (shake_test)\n");
  }
  printf("done\n");
  return 0;
}

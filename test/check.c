/* リリース前のチェックを行う */
#include <stdio.h>
#include <stdlib.h>
#include <anthy.h>

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
  if (shake_test("あいうえおかきくけこ")) {
    printf("fail (shake_test)\n");
  }
  printf("done\n");
  return 0;
}

/* texttrieのテスト用コード */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <texttrie.h>

static void
print_quoted(const char *key)
{
  if (!key) {
    printf("{null}");
    return ;
  }
  printf("\"");
  while (*key) {
    if (*key == '\"') {
      printf("\\\"");
    } else {
      printf("%c", *key);
    }
    key ++;
  }
  printf("\"");
}

static void
dump(const char *fn)
{
  struct text_trie *tt;
  char buf[32];
  char *key;
  tt = anthy_trie_open(fn, 0);
  buf[0] = 0;
  printf("#dump start\n");
  while ((key = anthy_trie_find_next_key(tt, buf, 32))) {
    char *val = anthy_trie_find(tt, key);
    print_quoted(key);
    printf(" ");
    print_quoted(val);
    printf("\n");
    free(val);
  }
  printf("#dump done\n");
  anthy_trie_close(tt);
}

static char *
get_word(char **p)
{
  char buf[1024];
  int idx = 0;
  while ((**p) != '\"') {
    (*p) ++;
  }
  (*p) ++;
  while ((**p) != '\"') {
    if (**p == '\\') {
      buf[idx] = (*p)[1];
    } else {
      buf[idx] = **p;
    }
    idx ++;
    (*p)++;
  }
  (*p)++;
  buf[idx] = 0;
  return strdup(buf);
}

static void
load_from_stdin(const char *fn)
{
  struct text_trie *tt;
  char buf[1024];
  tt = anthy_trie_open(fn, 0);
  while (fgets(buf, 1024, stdin)) {
    char *p = buf;
    char *key, *val;
    if (buf[0] != '\"') {
      continue;
    }
    key = get_word(&p);
    val = get_word(&p);
    anthy_trie_add(tt, key, val);
  }
  anthy_trie_close(tt);
}

/* test of data addition */
static void
test1(const char *fn)
{
  struct text_trie *tt;
  char buf[32];
  int i;
  tt = anthy_trie_open(fn, 0);
  for (i = 0; i < 1000; i++) {
    sprintf(buf, "%d", i);
    anthy_trie_add(tt, buf, buf);
  }
  anthy_trie_close(tt);
}

static void
test2(const char *fn)
{
  struct text_trie *tt;
  tt = anthy_trie_open(fn, 0);
  anthy_trie_add(tt, "ab", "xy");
  anthy_trie_add(tt, "abc", "xyz");
  anthy_trie_delete(tt, "ab");
  anthy_trie_close(tt);
}

static void
test3(const char *fn)
{
  struct text_trie *tt;
  char buf[10];
  tt = anthy_trie_open(fn, 0);
  anthy_trie_add(tt, "cb", "xy");
  anthy_trie_add(tt, "bc", "xyz");
  anthy_trie_add(tt, "cbc", "xyz");
  sprintf(buf, "cb");
  if (anthy_trie_find_next_key(tt, buf, 10)) {
    printf("(1)buf=(%s)\n", buf);
  } else {
    printf("(1)failed\n");
  }
  /**/
  sprintf(buf, "c");
  if (anthy_trie_find_next_key(tt, buf, 10)) {
    printf("(2)buf=(%s)\n", buf);
  } else {
    printf("(2)failed\n");
  }
  
  anthy_trie_close(tt);
}

int
main(int argc, char **argv)
{
  int i;
  for (i = 1; i < argc; i++) {
    char *arg = argv[i];
    char *next_arg = NULL;
    if (i < argc - 1) {
      next_arg = argv[i+1];
    }
    if (arg[0] != '-') {
      dump(arg);
      exit(0);
    } else if (!strcmp(arg, "--load")) {
      load_from_stdin(next_arg);
      exit(0);
    } else if (!strcmp(arg, "--test1")) {
      test1(next_arg);
      exit(0);
    } else if (!strcmp(arg, "--test2")) {
      test2(next_arg);
      exit(0);
    } else if (!strcmp(arg, "--test3")) {
      test3(next_arg);
      exit(0);
    }
  }
  return 0;
}

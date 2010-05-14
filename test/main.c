/* ライブラリの関数呼び出しのテスト */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <anthy.h>
#include <config.h>
#ifdef USE_UCS4
#include <langinfo.h>
#endif

/* Makefile の $(srcdir) (静的データファイルの基準ディレクトリ) */
#ifndef SRCDIR
# define SRCDIR "."
#endif
/* ビルド時のカレントディレクトリ (ここに .anthy を作る) */
#ifndef TEST_HOME
# define TEST_HOME "."		/* FIXME: 実際は相対パスだと誤動作する */
#endif

#define TESTDATA "test.txt"
const char *testdata = SRCDIR "/" TESTDATA;

struct input {
  char *str;
  char *kw;
  int serial;
} cur_input;

struct condition {
  int serial;
  int from;
  int to;
  char *kw;
  int loop;
  int silent;
} cond;

static int read_file(FILE *fp)
{
  char buf[256];
  while(fgets(buf, 256, fp)) {
    switch(buf[0]){
    case '#':
      break;
    case '*':
      if (cur_input.str) {
	free(cur_input.str);
	cur_input.str = 0;
      }
      buf[strlen(buf)-1] = 0;
      cur_input.str = strdup(&buf[1]);
      break;
    case ':':
      break;
    case '-':
      cur_input.serial ++;
      if (cur_input.kw) {
	free(cur_input.kw);
	cur_input.kw = 0;
      }
      return 0;
      break;
    }
  }
  return -1;
}

static int
check_cond(void)
{
  if (cur_input.serial == cond.serial) {
    return 1;
  }
  if (cur_input.kw && cond.kw && strstr(cur_input.kw, cond.kw)) {
    return 1;
  }
  if (cur_input.serial <= cond.to && cur_input.serial >= cond.from) {
    return 1;
  }
  return 0;
}

static void
init_lib(void)
{
  /* 既にインストールされているファイルの影響を受けないようにする */
  anthy_conf_override("CONFFILE", "../anthy-conf");
  anthy_conf_override("HOME", TEST_HOME);
  anthy_conf_override("DEPWORD", "master.depword");
  anthy_conf_override("DIC_FILE", "../mkanthydic/anthy.dic");
  anthy_conf_override("ANTHYDIR", SRCDIR "/../depgraph");
  if (anthy_init()) {
    printf("failed to init anthy\n");
    exit(0);
  }
  anthy_set_personality("");
#ifdef USE_UCS4
  /* 表示するためのエンコーディングを設定する */
  anthy_context_set_encoding(NULL, ANTHY_UTF8_ENCODING);
#endif
}

static void
print_usage(void)
{
  printf("Anthy "VERSION"\n"
	 "./anthy [test-id]\n"
	 " For example.\n"
	 "  ./anthy 1\n"
	 "  ./anthy --to 100\n"
	 "  ./anthy --from 10 --to 100\n"
	 "  ./anthy --loop\n"
	 "  ./anthy --silent\n"
	 "  ./anthy --all\n"
	 "  ./anthy --ll 1\n\n");
  exit(0);
}

static void
parse_args(int argc, char **argv)
{
  int i;
  char *arg;
  for (i = 1; i < argc; i++) {
    arg = argv[i];
    if (!strncmp(arg, "--", 2)) {
      arg = &arg[2];
      if (!strcmp(arg, "help") || !strcmp(arg, "version")) {
	print_usage();
      }
      if (!strcmp(arg, "all")) {
	cond.from = 0;
	cond.to = 100000000;
      } else if (!strcmp(arg, "loop")) {
	cond.loop = 1;
      } else if (!strcmp(arg, "silent")) {
	cond.silent = 1;
      }
      if (i + 1 < argc) {
	if (!strcmp(arg, "from")){
	  cond.from = atoi(argv[i+1]);
	  i++;
	}else if (!strcmp(arg, "to")){
	  cond.to = atoi(argv[i+1]);
	  i++;
	}else if (!strcmp(arg, "ll")) {
	  anthy_set_logger(NULL, atoi(argv[i+1]));
	  i++;
	}
      }
    } else {
      int num = atoi(arg);
      if (num) {
	cond.serial = num;
      } else {
	char *buf = alloca(strlen(SRCDIR)+strlen(arg) + 10);
	sprintf(buf, SRCDIR "/%s.txt", arg);
	testdata = strdup(buf);
      }
    }
  }
}

static void
print_run_env(void)
{
  time_t t;
  const char *env;
  env = getenv("ANTHY_ENABLE_DEBUG_PRINT");
  if (!env) {
    env = "";
  }
  printf("ANTHY_ENABLE_DEBUG_PRINT=(%s)\n", env);
  env = getenv("ANTHY_SPLITTER_PRINT");
  if (!env) {
    env = "";
  }
  printf("ANTHY_SPLITTER_PRINT=(%s)\n", env);
  printf("SRCDIR=(%s)\n", SRCDIR);
  t = time(&t);
  printf(PACKAGE "-" VERSION " %s", ctime(&t));
}

int
main(int argc,char **argv)
{
  anthy_context_t ac;
  FILE *fp;

  parse_args(argc,argv);
  printf("./test_anthy --help to print usage.\n");

  print_run_env();

  init_lib();

 begin:;
  fp = fopen(testdata, "r");
  if (!fp) {
    printf("failed to open %s.\n", testdata);
    return 0;
  }
  
  ac = anthy_create_context();
  anthy_context_set_encoding(ac, ANTHY_EUC_JP_ENCODING);

  while (!read_file(fp)) {
    if (check_cond()) {
      anthy_set_string(ac,cur_input.str);
      if (!cond.silent) {
	printf("%d:(%s)\n", cur_input.serial, cur_input.str);
	anthy_print_context(ac);
      }
      anthy_reset_context(ac);
    }
  }
  anthy_release_context(ac);

  if (cond.loop) {
    fclose(fp);
    goto begin;
  }
  anthy_quit();

  return 0;
}

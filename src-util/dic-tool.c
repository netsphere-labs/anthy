/*
 * 辞書操作用のユーティリティコマンド
 *
 * 辞書のライブラリ内部の形式と外部の形式の相互変換を行う
 * 外部形式は
 * *読み 頻度 単語
 * *品詞の変数1 = 値1
 * *品詞の変数2 = 値2
 * *...
 * *<空行>
 * になる
 */
/*
 * Funded by IPA未踏ソフトウェア創造事業 2001 9/22
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dicutil.h>
#include <config.h>

#define UNSPEC 0
#define DUMP_DIC 1
#define LOAD_DIC 2
#define APPEND_DIC 3

#define TYPETAB "typetab"
#define USAGE_TEXT "dic-tool-usage.txt"

#define USAGE \
 "Anthy-dic-util [options]\n"\
 " --help: Show this usage text\n"\
 " --version: Show version\n"\
 " --dump: Dump dictionary\n"\
 " --load: Load dictionary\n"\
 " --append: Append dictionary\n"\
 " --personality=NAME: use NAME as a name of personality\n"


static int command = UNSPEC;
static FILE *fp_in;
static char *fn;
static const char *personality = "";

/* 変数名と値のペア */
struct var{
  struct var *next;
  char *var_name;
  char *val;
};

struct trans_tab{
  struct trans_tab *next;
  char *type_name; /* 内部での型の名前 T35とか */
  struct var var_list; /* 型を決定するためのパラメータ */
}trans_tab_list;

static void
print_usage(void)
{
  printf(USAGE);
  exit(0);
}

static FILE *
open_typetab(void)
{
  FILE *fp;
  char *fn;
  fp = fopen(TYPETAB, "r");
  if (fp) {
    return fp;
  }
  fn = strdup(anthy_dic_util_get_anthydir());
  fn = realloc(fn, strlen(fn) + strlen(TYPETAB) + 4);
  strcat(fn, "/");
  strcat(fn, TYPETAB);
  fp = fopen(fn, "r");
  return fp;
}

static void
print_usage_text(void)
{
  FILE *fp;
  char buf[256];
  int c;
  fp = fopen(USAGE_TEXT, "r");
  if (!fp) {
    char *fn;
    fn = strdup(anthy_dic_util_get_anthydir());
    fn = realloc(fn, strlen(fn) + strlen(USAGE_TEXT) + 10);
    strcat(fn, "/" USAGE_TEXT);
    fp = fopen(fn, "r");
  }
  if (!fp) {
    printf("# Anthy-dic-tool\n#\n");
    return ;
  }
  fprintf(stdout, "#" PACKAGE " " VERSION "\n");
  while ((c = fread(buf, 1, 256, fp)) > 0) {
    fwrite(buf, 1, c, stdout);
  }
  fclose(fp);
}

static char *
read_line(char *buf, int len, FILE *fp)
{
  while (fgets(buf, len, fp)) {
    if (buf[0] != '#') {
      int l = strlen(buf);
      if (l > 0 && buf[l-1] == '\n') {
	buf[l-1] = 0;
      }
      return buf;
    }
  }
  return NULL;
}

static int
read_var(struct var *head, FILE *fp)
{
  char buf[256];
  char var[256], eq[256], val[256];
  struct var *v;
  if (!read_line(buf, 256, fp)) {
    return -1;
  }
  if (sscanf(buf, "%s %s %s", var, eq, val) != 3) {
    return -1;
  }

  v = malloc(sizeof(struct var));
  v->next = head->next;
  head->next = v;

  v->var_name = strdup(var);
  v->val = strdup(val);
  return 0;
}

static int
read_entry(FILE *fp)
{
  char buf[256], type_name[256];
  char *res;
  struct trans_tab *t;
  do {
    res = read_line(buf, 256, fp);
    if (!res) {
      return -1;
    }
  } while (res[0] == '#' || res[0] == 0);
  t = malloc(sizeof(struct trans_tab));
  sprintf(type_name, "#%s", buf);
  t->type_name = strdup(type_name);
  t->var_list.next = 0;
  while(!read_var(&t->var_list, fp));
  t->next = trans_tab_list.next;
  trans_tab_list.next = t;
  return 0;
}

static void
read_typetab(void)
{
  FILE *fp = open_typetab();
  if (!fp) {
    printf("Failed to open type table.\n");
    exit(1);
  }
  while (!read_entry(fp));
}

static struct trans_tab *
find_trans_tab_by_name(char *name)
{
  struct trans_tab *t;
  for (t = trans_tab_list.next; t; t = t->next) {
    if (!strcmp(t->type_name, name)) {
      return t;
    }
  }
  return NULL;
}

static void
print_word_type(struct trans_tab *t)
{
  struct var *v;
  for (v = t->var_list.next; v; v = v->next) {
    printf("%s\t=\t%s\n", v->var_name, v->val);
  }
}

static void
dump_dic(void)
{
  print_usage_text();
  if (anthy_priv_dic_select_first_entry() == -1) {
    printf("# Failed to read private dictionary\n"
	   "# There are no words or error occured?\n"
	   "#\n");
    return ;
  }
  do {
    char idx[100], wt[100], w[100];
    int freq;
    if (anthy_priv_dic_get_index(idx, 100) &&
	anthy_priv_dic_get_wtype(wt, 100) &&
	anthy_priv_dic_get_word(w, 100)) {
      struct trans_tab *t;
      freq = anthy_priv_dic_get_freq();
      t = find_trans_tab_by_name(wt);
      if (t) {
	printf("%s %d %s\n", idx, freq, w);
	print_word_type(t);
	printf("\n");
      } else {
	printf("# Failed to determine word type of %s(%s).\n", w, wt);
      }
    }
  } while (anthy_priv_dic_select_next_entry() == 0);
}

static void
open_input_file(void)
{
  if (!fn) {
    fp_in = stdin;
  } else {
    fp_in = fopen(fn, "r");
    if (!fp_in) {
      exit(1);
    }
  }
}

/* vが sの中にあるか */
static int
match_var(struct var *v, struct var *s)
{
  struct var *i;
  for (i = s->next; i; i = i->next) {
    if (!strcmp(v->var_name, i->var_name) &&
	!strcmp(v->val, i->val)) {
      return 1;
    }
  }
  return 0;
}

/* v1がv2の部分集合かどうか */
static int
var_list_subset_p(struct var *v1, struct var *v2)
{
  struct var *v;
  for (v = v1->next; v; v = v->next) {
    if (!match_var(v, v2)) {
      return 0;
    }
  }
  return 1;
}

static char *
find_wt(void)
{
  struct var v;
  struct trans_tab *t;
  v.next = 0;
  while(!read_var(&v, fp_in));
  for (t = trans_tab_list.next; t; t = t->next) {
    if (var_list_subset_p(&t->var_list, &v) &&
	var_list_subset_p(&v, &t->var_list)) {
      return t->type_name;
    }
  }
  return NULL;
}

static int
find_head(char *yomi, char *freq, char *w)
{
  char buf[256];
  do {
    if (!read_line(buf, 256, fp_in)) {
      return -1;
    }
  } while (sscanf(buf, "%s %s %s",yomi, freq, w) != 3);
  return 0;
}

static void
load_dic(void)
{
  char yomi[256], freq[256], w[256];
  while (!find_head(yomi, freq, w)) {
    char *wt = find_wt();
    if (wt) {
      int ret;
      ret = anthy_priv_dic_add_entry(yomi, w, wt, atoi(freq));
      if (ret == -1) {
	printf("Failed to register %s\n", yomi);
      }else {
	printf("Word %s is registered as %s\n", yomi, wt);
      }
    } else {
      printf("Failed to find the type of %s.\n", yomi);
    }
  }
}

static void
print_version(void)
{
  printf("Anthy-dic-util "VERSION".\n");
  exit(0);
}

static void
parse_args(int argc, char **argv)
{
  int i;
  for (i = 1 ; i < argc ; i++) {
    if (!strncmp(argv[i], "--", 2)) {
      char *opt = &argv[i][2];
      if (!strcmp(opt, "help")) {
	print_usage();
      } else if (!strcmp(opt, "version")){
	print_version();
      } else if (!strcmp(opt, "dump")) {
	command = DUMP_DIC;
      } else if (!strcmp(opt,"append") ){
	command = APPEND_DIC;
      } else if (!strncmp(opt, "personality=", 12)) {
	personality = &opt[12];
      } else if (!strcmp(opt, "load")) {
	command = LOAD_DIC;
      }
    }else{
      fn = argv[i];
    }
  }
}

int
main(int argc,char **argv)
{
  fp_in = stdin;
  parse_args(argc, argv);

  switch (command) {
  case DUMP_DIC:
    anthy_dic_util_init();
    read_typetab();
    dump_dic();
    break;
  case LOAD_DIC:
    anthy_dic_util_init();
    read_typetab();
    anthy_priv_dic_delete();
    open_input_file();
    load_dic();
    break;
  case APPEND_DIC:
    anthy_dic_util_init();
    read_typetab();
    open_input_file();
    load_dic();
    break;
  case UNSPEC:
  default:
    print_usage();
  }
  return 0;
}

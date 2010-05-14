/*
 * 付属語グラフをバイナリ化する
 * init_word_seq_tab()
 *   付属語テーブル中のノードへのポインタの初期化
 * release_depword_tab()
 *   付属語テーブルの解放
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <alloc.h>
#include <conf.h>
#include <ruleparser.h>
#include <xstr.h>
#include <logger.h>
#include <splitter.h>
#include <anthy.h>
#include <depgraph.h>
#include <diclib.h>

#ifndef SRCDIR
#define SRCDIR "."
#endif

/* メモリ節約のために文字列を共有するpool */
static struct {
  int nr_xs;
  xstr **xs;
} xstr_pool ;

static struct dep_node* gNodes;
static char** gNodeNames;
static int nrNodes;

static allocator wordseq_rule_ator;


/* 単語接続ルール */
static struct wordseq_rule *gRules;
int nrRules;


#define  NORMAL_CONNECTION 1
#define  WEAKER_CONNECTION 2
#define  WEAK_CONNECTION 8

static int 
get_node_id_by_name(const char *name)
{
  int i;
  /* 登録済みのものから探す */
  for (i = 0; i < nrNodes; i++) {
    if (!strcmp(name,gNodeNames[i])) {
      return i;
    }
  }
  /* なかったので作る */
  gNodes = realloc(gNodes, sizeof(struct dep_node)*(nrNodes+1));
  gNodeNames = realloc(gNodeNames, sizeof(char*)*(nrNodes+1));
  gNodes[nrNodes].nr_branch = 0;
  gNodes[nrNodes].branch = 0;
  gNodeNames[nrNodes] = strdup(name);
  nrNodes++;
  return nrNodes-1;
}


/* 遷移条件からbranchを捜し出す */
static struct dep_branch *
find_branch(struct dep_node *node, xstr **strs, int nr_strs)
{
  struct dep_branch *db;
  int i, j;
  /* 同じ遷移条件のブランチを探す */
  for (i = 0; i < node->nr_branch; i++) {
    db = &node->branch[i];
    if (nr_strs != db->nr_strs) {
      continue ;
    }
    for (j = 0; j < nr_strs; j++) {
      if (anthy_xstrcmp(db->str[j], strs[j])) {
	goto fail;
      }
    }
    /**/
    return db;
  fail:;
  }
  /* 新しいブランチを確保する */
  node->branch = realloc(node->branch,
			 sizeof(struct dep_branch)*(node->nr_branch+1));
  db = &node->branch[node->nr_branch];
  node->nr_branch++;
  db->str = malloc(sizeof(xstr*)*nr_strs);
  for (i = 0; i < nr_strs; i++) {
    db->str[i] = strs[i];
  }
  db->nr_strs = nr_strs;
  db->nr_transitions = 0;
  db->transition = 0;
  return db;
}

/* 文字列poolから文字列を検索する */
static xstr *
get_xstr_from_pool(char *str)
{
  int i;
  xstr *xs;
#ifdef USE_UCS4
  xs = anthy_cstr_to_xstr(str, ANTHY_EUC_JP_ENCODING);
#else
  xs = anthy_cstr_to_xstr(str, ANTHY_COMPILED_ENCODING);
#endif
  /* poolに既にあるか探す */
  for (i = 0; i < xstr_pool.nr_xs; i++) {
    if (!anthy_xstrcmp(xs, xstr_pool.xs[i])) {
      anthy_free_xstr(xs);
      return xstr_pool.xs[i];
    }
  }
  /* 無いので、作る */
  xstr_pool.xs = realloc(xstr_pool.xs,
			 sizeof(xstr *) * (xstr_pool.nr_xs+1));
  xstr_pool.xs[xstr_pool.nr_xs] = xs;
  xstr_pool.nr_xs ++;
  return xs;
}

static void
release_xstr_pool(void)
{
  int i;
  for (i = 0; i < xstr_pool.nr_xs; i++) {
    free(xstr_pool.xs[i]->str);
    free(xstr_pool.xs[i]);
  }
  free(xstr_pool.xs);
  xstr_pool.nr_xs = 0;
}

/*
 * 遷移をparseする
 *  doc/SPLITTER参照
 */
static void
parse_transition(char *token, struct dep_transition *tr)
{
  int conn = NORMAL_CONNECTION;
  int ct = CT_NONE;
  int pos = POS_NONE;
  enum dep_class dc = DEP_NONE;
  char *str = token;
  tr->head_pos = POS_NONE;
  tr->weak = 0;
  /* 遷移の属性を解析*/
  while (*token != '@') {
    switch(*token){
    case ':':
      conn = WEAKER_CONNECTION;
      tr->weak = 1;
      break;
    case '.':
      conn = WEAK_CONNECTION;
      tr->weak = 1;
      break;
    case 'C':
      /* 活用形 */
      switch (token[1]) {
      case 'z': ct = CT_MIZEN; break;
      case 'y': ct = CT_RENYOU; break;
      case 's': ct = CT_SYUSI; break;
      case 't': ct = CT_RENTAI; break;
      case 'k': ct = CT_KATEI; break;
      case 'm': ct = CT_MEIREI; break;
      case 'g': ct = CT_HEAD; break;
      }
      token ++;
      break;
    case 'H':
      /* 自立語部の品詞 */
      switch (token[1]) {
      case 'n':	tr->head_pos = POS_NOUN; break;
      case 'v':	tr->head_pos = POS_V; break;
      case 'j':	tr->head_pos = POS_AJV; break;
      }
      token ++;
      break;
    case 'S':
      /* 文節の属性 */
      switch (token[1]) {
	/*      case 'n': sc = DEP_NO; break;*/
      case 'f': dc = DEP_FUZOKUGO; break;
      case 'k': dc = DEP_KAKUJOSHI; break;
      case 'y': dc = DEP_RENYOU; break;
      case 't': dc = DEP_RENTAI; break;
      case 'e': dc = DEP_END; break;
      case 'r': dc = DEP_RAW; break;
      default: printf("unknown (S%c)\n", token[1]);
      }
      token ++;
      break;
    default:
      printf("Unknown (%c) %s\n", *token, str);
      break;
    }
    token ++;
  }
  /* @から後はノードの名前 */
  tr->next_node = get_node_id_by_name(token);
  /* 接続の強さ */
  tr->trans_ratio = RATIO_BASE / conn;
  /**/
  tr->pos = pos;
  tr->ct = ct;
  tr->dc = dc;
}

/*
 * ノード名 遷移条件+ 遷移先+
 */
static void
parse_dep(char **tokens, int nr)
{
  int id, row = 0;
  struct dep_branch *db;
  struct dep_node *dn;
  int nr_strs;
  xstr **strs = alloca(sizeof(xstr*) * nr);

  /* ノードとそのidを確保 */
  id = get_node_id_by_name(tokens[row]);
  dn = &gNodes[id];
  row ++;

  nr_strs = 0;

  /* 遷移条件の付属語の配列を作る */
  for (; row < nr && tokens[row][0] == '\"'; row++) {
    char *s;
    s = strdup(&tokens[row][1]);
    s[strlen(s)-1] =0;
    strs[nr_strs] = get_xstr_from_pool(s);
    nr_strs ++;
    free(s);
  }

  /* 遷移条件がない時は警告を出して、空の遷移条件を追加する */
  if (nr_strs == 0) {
    char *s;
    anthy_log(0, "node %s has a branch without any transition condition.\n",
	      tokens[0]);
    s = strdup("");
    strs[0] = get_xstr_from_pool(s);
    nr_strs = 1;
    free(s);
  }

  /* ブランチに遷移先のノードを追加する */
  db = find_branch(dn, strs, nr_strs);
  for ( ; row < nr; row++){
    db->transition = realloc(db->transition,
			     sizeof(struct dep_transition)*
			     (db->nr_transitions+1));
    parse_transition(tokens[row], &db->transition[db->nr_transitions]);
    db->nr_transitions ++;
  }
}

/* 文法定義ファイル中に空のノードがあるかチェックする */
static void
check_nodes(void)
{
  int i;
  for (i = 1; i < nrNodes; i++) {
    if (gNodes[i].nr_branch == 0) {
      anthy_log(0, "node %s has no branch.\n", gNodeNames);
    }
  }
}


static int
init_depword_tab(void)
{
  const char *fn;
  char **tokens;
  int nr;

  xstr_pool.nr_xs = 0;
  xstr_pool.xs = NULL;

  /* id 0 を空ノードに割当てる */
  get_node_id_by_name("@");

  fn = anthy_conf_get_str("DEPWORD");
  if (!fn) {
    anthy_log(0, "Dependent word dictionary is unspecified.\n");
    return -1;
  }
  if (anthy_open_file(fn) == -1) {
    anthy_log(0, "Failed to open dep word dict (%s).\n", fn);
    return -1;
  }
  /* 一行ずつ付属語グラフを読む */
  while (!anthy_read_line(&tokens, &nr)) {
    parse_dep(tokens, nr);
    anthy_free_line();
  }
  anthy_close_file();
  check_nodes();
  return 0;
}


static void
parse_indep(char **tokens, int nr)
{
  int tmp;
  if (nr < 2) {
    printf("Syntex error in indepword defs"
	   " :%d.\n", anthy_get_line_number());
    return ;
  }
  gRules = realloc(gRules, sizeof(struct wordseq_rule)*(nrRules+1));

  /* 行の先頭には品詞の名前が入っている */
  gRules[nrRules].wt = anthy_init_wtype_by_name(tokens[0]);

  /* 比率 */
  tmp = atoi(tokens[1]);
  if (tmp == 0) {
    tmp = 1;
  }
  gRules[nrRules].ratio = RATIO_BASE - tmp*(RATIO_BASE/64);

  /* その次にはノード名が入っている */
  gRules[nrRules].node_id = get_node_id_by_name(tokens[2]);

  ++nrRules;
}

/** 付属語グラフを読み込む */
static int 
init_word_seq_tab(void)
{
  const char *fn;
  char **tokens;
  int nr;

  wordseq_rule_ator = anthy_create_allocator(sizeof(struct wordseq_rule),
					     NULL);

  fn = anthy_conf_get_str("INDEPWORD");
  if (!fn){
    printf("independent word dict unspecified.\n");
    return -1;
  }
  if (anthy_open_file(fn) == -1) {
    printf("Failed to open indep word dict (%s).\n", fn);
    return -1;
  }
  /* ファイルを一行ずつ読む */
  while (!anthy_read_line(&tokens, &nr)) {
    parse_indep(tokens, nr);
    anthy_free_line();
  }
  anthy_close_file();

  return 0;
}

/*  
    ネットワークバイトオーダーで4byte書き出す
*/
static void
write_nl(FILE* fp, int i)
{
  i = anthy_dic_htonl(i);
  fwrite(&i, sizeof(int), 1, fp);
}

static void
write_transition(FILE* fp, struct dep_transition* transition)
{
  write_nl(fp, transition->next_node); 
  write_nl(fp, transition->trans_ratio); 
  write_nl(fp, transition->pos); 
  write_nl(fp, transition->ct); 
  write_nl(fp, transition->dc); 
  write_nl(fp, transition->head_pos); 
  write_nl(fp, transition->weak); 
}

static void
write_xstr(FILE* fp, xstr* str)
{
  write_nl(fp, str->len);
  fwrite(str->str, sizeof(xchar), str->len, fp);
}

static void
write_branch(FILE* fp, struct dep_branch* branch)
{
  int i;

  write_nl(fp, branch->nr_strs);
  for (i = 0; i < branch->nr_strs; ++i) {
    write_xstr(fp, branch->str[i]);
  }

  write_nl(fp, branch->nr_transitions);
  for (i = 0; i < branch->nr_transitions; ++i) {
    write_transition(fp, &branch->transition[i]);
  }
}

static void
write_node(FILE* fp, struct dep_node* node)
{
  int i;
  write_nl(fp, node->nr_branch);
  for (i = 0; i < node->nr_branch; ++i) {
    write_branch(fp, &node->branch[i]);
  }
}

static void
write_wtype(FILE *fp, wtype_t wt)
{
  fputc(anthy_wtype_get_pos(wt), fp);
  fputc(anthy_wtype_get_cos(wt), fp);
  fputc(anthy_wtype_get_scos(wt), fp);
  fputc(anthy_wtype_get_cc(wt), fp);
  fputc(anthy_wtype_get_ct(wt), fp);
  fputc(anthy_wtype_get_wf(wt), fp);
  fputc(0, fp);
  fputc(0, fp);
}

static void
write_file(const char* file_name)
{
  int i;
  FILE* fp = fopen(file_name, "w");
  int* node_offset = malloc(sizeof(int) * nrNodes); /* gNodesのファイル上の位置 */

  write_nl(fp, nrRules);
  for (i = 0; i < nrRules; ++i) {
    write_wtype(fp, gRules[i].wt);
    write_nl(fp, gRules[i].ratio);
    write_nl(fp, gRules[i].node_id);
  }

  write_nl(fp, nrNodes);

  for (i = 0; i < nrNodes; ++i) {
    write_node(fp, &gNodes[i]);
  }

  free(node_offset);
  fclose(fp);
}

static void
release_depword_tab(void)
{
  int i, j;
  for (i = 0; i < nrNodes; i++) {
    for (j = 0; j < gNodes[i].nr_branch; j++) {
      free(gNodes[i].branch[j].str);
      free(gNodes[i].branch[j].transition);
    }
    free(gNodes[i].branch);
  }
  free(gNodes);
  gNodes = 0;
  nrNodes = 0;

  release_xstr_pool();
}


int
main(int argc, char* argv[])
{
  /* 付属語辞書を読み込んでファイルに書き出す */
  anthy_conf_override("CONFFILE", "../anthy-conf");
  anthy_conf_override("ANTHYDIR", SRCDIR "/../depgraph/");

  anthy_init_wtypes();
  anthy_do_conf_init();
  init_depword_tab();
  init_word_seq_tab();

  write_file("anthy.dep");

  release_depword_tab();

  return 0;
}

/*
 * 文節の自立語部(接頭辞、接尾辞含む)に続く
 * 助詞、助動詞などの付属語のパターンをたどる。
 * パターンはグラフとして設定ファイルに用意する。
 *
 *
 *  +------+
 *  |      |
 *  |branch+--cond--+--transition--> node
 *  |      |        +--transition--> node
 *  | NODE |
 *  |      |
 *  |branch+--cond-----transition--> node
 *  |      |
 *  |branch+--cond-----transition--> node
 *  |      |
 *  +------+
 *
 * Copyright (C) 2000-2003 TABATA Yusuke
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include <anthy.h>

#include <conf.h>
#include <ruleparser.h>
#include <xstr.h>
#include <logger.h>
#include <segclass.h>
#include <splitter.h>
#include <wtype.h>
#include "wordborder.h"

static int nrNodes;

#define  NORMAL_CONNECTION 1
#define  WEAKER_CONNECTION 2
#define  WEAK_CONNECTION 8

struct dep_transition {
  /** 遷移先のノードの番号 0の場合は終端 */
  int next_node;
  /** 遷移のスコア */
  int trans_ratio;
  /** 品詞 */
  int pos;
  /** 活用形 */
  int ct;
  /* 付属語クラス */
  enum dep_class dc;

  int head_pos;
  int weak;
};

struct dep_branch {
  /* 遷移条件の付属語の配列 */
  /** 配列長 */
  int nr_strs;
  /** 遷移条件の配列 */
  xstr **str;
  
  /** 遷移先のノード */
  int nr_transitions;
  struct dep_transition *transition;
};

static struct dep_node {
  /** ノードの名前 */
  char *name;

  int nr_branch;
  struct dep_branch *branch;
}*gNodes;

/* メモリ節約のために文字列を共有するpool */
static struct {
  int nr_xs;
  xstr **xs;
} xstr_pool ;

static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, xstr *cond_xs, struct dep_branch *db);
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node);

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

/* 文法定義ファイル中に空のノードがあるかチェックする */
static void
check_nodes(void)
{
  int i;
  for (i = 1; i < nrNodes; i++) {
    if (gNodes[i].nr_branch == 0) {
      anthy_log(0, "node %s has no branch.\n", gNodes[i].name);
    }
  }
}


/*
 * 各ノードにおける遷移条件をテストする
 *
 * wl 自立語部のword_list
 * follow_str 自立語部以降の文字列
 * node ルールの番号
 */
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node)
{
  struct dep_node *dn = &gNodes[node];
  struct dep_branch *db;
  int i,j;

  /* 各ルールの */
  for (i = 0; i < dn->nr_branch; i++) {
    db = &dn->branch[i];
    /* 各遷移条件 */
    for (j = 0; j < db->nr_strs; j++) {
      xstr cond_xs;
      /* 付属語の方が遷移条件より長いことが必要 */
      if (follow_str.len < db->str[j]->len){
	continue;
      }
      /* 遷移条件の部分を切り出す */
      cond_xs.str = follow_str.str;
      cond_xs.len = db->str[j]->len;

      /* 遷移条件と比較する */
      if (!anthy_xstrcmp(&cond_xs, db->str[j])) {
	/* 遷移条件にmatchした */
	struct word_list new_wl = *wl;
	struct part_info *part = &new_wl.part[PART_DEPWORD];
	xstr new_follow;

	part->len += cond_xs.len;
	new_follow.str = &follow_str.str[cond_xs.len];
	new_follow.len = follow_str.len - cond_xs.len;
	/* 遷移してみる */
	match_branch(sc, &new_wl, &new_follow, &cond_xs, db);
      }
    }
  }
}

/*
 * 各遷移を実行してみる
 *
 * tmpl ここまでに構成したword_list
 * xs 残りの文字列
 * cond_xs 遷移に使われた文字列
 * db 現在調査中のbranch
 */
static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, xstr *cond_xs, struct dep_branch *db)
{
  struct part_info *part = &tmpl->part[PART_DEPWORD];
  int i;

  /* 遷移先を順にトライする */
  for (i = 0; i < db->nr_transitions; i++) {
    int conn_ratio = part->ratio; /* scoreを保存 */
    int weak_len = tmpl->weak_len;/* weakな遷移の長さを保存*/ 
    int head_pos = tmpl->head_pos; /* 品詞の情報 */
    enum dep_class dc = part->dc;
    struct dep_transition *transition = &db->transition[i];

    /* この遷移のスコア */
    part->ratio *= transition->trans_ratio;
    part->ratio /= RATIO_BASE;
    if (transition->weak || /* 弱い遷移 */
	(transition->dc == DEP_END && xs->len > 0)) { /* 終端じゃないのに終端属性*/
      tmpl->weak_len += cond_xs->len;
    } else {
      /* 強い遷移の付属語に加点 */
      part->ratio += cond_xs->len * cond_xs->len * cond_xs->len * 3;
    }

    tmpl->tail_ct = transition->ct;
    /* 遷移の活用形と品詞 */
    if (transition->dc != DEP_NONE) {
      part->dc = transition->dc;

    }
    /* 名詞化する動詞等で品詞名を上書き */
    if (transition->head_pos != POS_NONE) {
      tmpl->head_pos = transition->head_pos;
    }

    /* 遷移か終端か */
    if (transition->next_node) {
      /* 遷移 */
      match_nodes(sc, tmpl, *xs, transition->next_node);
    } else {
      struct word_list *wl;
      xstr xs_tmp;

      /* 
       * 終端ノードに到達したので、
       * それをword_listとしてコミット
       */
      wl = anthy_alloc_word_list(sc);
      *wl = *tmpl;
      wl->len += part->len;

      /* 一文字の付属語で強い接続のものかどうかを判定する */
      xs_tmp = *xs;
      xs_tmp.str--;
      if (wl->part[PART_DEPWORD].len == 1 &&
	  (anthy_get_xchar_type(xs_tmp.str[0]) & XCT_STRONG)) {
	wl->part[PART_DEPWORD].ratio *= 3;
	wl->part[PART_DEPWORD].ratio /= 2;
      }
      /**/
      anthy_commit_word_list(sc, wl);
    }
    /* 書き戻し */
    part->ratio = conn_ratio;
    part->dc = dc;
    tmpl->weak_len = weak_len;
    tmpl->head_pos = head_pos;
  }
}

/** 検索開始
 */
void
anthy_scan_node(struct splitter_context *sc,
		struct word_list *tmpl,
		xstr *follow, int node)
{
  /* 付属語の付いていない状態から検索を開始する */
  match_nodes(sc, tmpl, *follow, node);
}

int
anthy_get_node_id_by_name(const char *name)
{
  int i;
  /* 登録済みのものから探す */
  for (i = 0; i < nrNodes; i++) {
    if (!strcmp(name,gNodes[i].name)) {
      return i;
    }
  }
  /* なかったので作る */
  gNodes = realloc(gNodes, sizeof(struct dep_node)*(nrNodes+1));
  gNodes[nrNodes].name = strdup(name);
  gNodes[nrNodes].nr_branch = 0;
  gNodes[nrNodes].branch = 0;
  nrNodes++;
  return nrNodes-1;
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
  tr->next_node = anthy_get_node_id_by_name(token);
  /* 接続の強さ */
  tr->trans_ratio = RATIO_BASE / conn;
  /**/
  tr->pos = pos;
  tr->ct = ct;
  tr->dc = dc;
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

/*
 * ノード名 遷移条件+ 遷移先+
 */
static void
parse_line(char **tokens, int nr)
{
  int id, row = 0;
  struct dep_branch *db;
  struct dep_node *dn;
  int nr_strs;
  xstr **strs = alloca(sizeof(xstr*) * nr);

  /* ノードとそのidを確保 */
  id = anthy_get_node_id_by_name(tokens[row]);
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

int
anthy_init_depword_tab()
{
  const char *fn;
  char **tokens;
  int nr;

  xstr_pool.nr_xs = 0;
  xstr_pool.xs = NULL;

  /* id 0 を空ノードに割当てる */
  anthy_get_node_id_by_name("@");

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
    parse_line(tokens, nr);
    anthy_free_line();
  }
  anthy_close_file();
  check_nodes();
  return 0;
}

void
anthy_release_depword_tab(void)
{
  int i, j;
  for (i = 0; i < nrNodes; i++) {
    free(gNodes[i].name);
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

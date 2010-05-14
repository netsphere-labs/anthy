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
 * Copyright (C) 2005 YOSHIDA Yuichi
 * Copyright (C) 2000-2005 TABATA Yusuke
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

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

/* 遷移グラフ */
static struct file_dep fdep;

#define  NORMAL_CONNECTION 1
#define  WEAKER_CONNECTION 2
#define  WEAK_CONNECTION 8


static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, xstr *cond_xs, struct dep_branch *db);
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node);



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
  struct dep_node *dn = &fdep.nodes[node];
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
    part->ratio *= ntohl(transition->trans_ratio);
    part->ratio /= RATIO_BASE;
    if (ntohl(transition->weak) || /* 弱い遷移 */
	(ntohl(transition->dc) == DEP_END && xs->len > 0)) { /* 終端じゃないのに終端属性*/
      tmpl->weak_len += cond_xs->len;
    } else {
      /* 強い遷移の付属語に加点 */
      part->ratio += cond_xs->len * cond_xs->len * cond_xs->len * 3;
    }

    tmpl->tail_ct = ntohl(transition->ct);
    /* 遷移の活用形と品詞 */
    if (ntohl(transition->dc) != DEP_NONE) {
      part->dc = ntohl(transition->dc);

    }
    /* 名詞化する動詞等で品詞名を上書き */
    if (ntohl(transition->head_pos) != POS_NONE) {
      tmpl->head_pos = ntohl(transition->head_pos);
    }

    /* 遷移か終端か */
    if (ntohl(transition->next_node)) {
      /* 遷移 */
      match_nodes(sc, tmpl, *xs, ntohl(transition->next_node));
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




static void
read_xstr(struct file_dep* fdep, xstr* str, int* offset)
{
  str->len = ntohl(*(int*)&fdep->file_ptr[*offset]);
  *offset += sizeof(int);
  str->str = (xchar*)&fdep->file_ptr[*offset];
  *offset += sizeof(xchar) * str->len;
}

static void
read_branch(struct file_dep* fdep, struct dep_branch* branch, int* offset)
{
  int i;

  branch->nr_strs = ntohl(*(int*)&fdep->file_ptr[*offset]);
  *offset += sizeof(int);
  branch->str = malloc(sizeof(xstr*) * branch->nr_strs);

  for (i = 0; i < branch->nr_strs; ++i) {
    xstr* str = malloc(sizeof(xstr));
    read_xstr(fdep, str, offset);
    branch->str[i] = str;
  }

  branch->nr_transitions = ntohl(*(int*)&fdep->file_ptr[*offset]);
  *offset += sizeof(int);
  branch->transition = (struct dep_transition*)&fdep->file_ptr[*offset];
  *offset += sizeof(struct dep_transition) * branch->nr_transitions;
}

static void
read_node(struct file_dep* fdep, struct dep_node* node, int* offset)
{
  int i;
  node->nr_branch = ntohl(*(int*)&fdep->file_ptr[*offset]);
  *offset += sizeof(int);
    
  node->branch = malloc(sizeof(struct dep_branch) * node->nr_branch);
  for (i = 0; i < node->nr_branch; ++i) {
    read_branch(fdep, &node->branch[i], offset);
  }
}

static int
map_file_dep(const char* file_name, struct file_dep* fdep)
{
  struct stat st;
  char* ptr;
  int fd, r;

  fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    anthy_log(0, "Failed to open (%s).\n", file_name);
    return -1;
  }
  r = fstat(fd, &st);
  if (r == -1) {
    anthy_log(0, "Failed to stat() (%s).\n", file_name);
    return -1;
  }
  fdep->file_size = st.st_size;
  ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (ptr == MAP_FAILED) {
    anthy_log(0, "Failed to mmap() (%s).\n", file_name);
    return -1;
  }
  fdep->file_ptr = ptr;
  return 0;
}

static void
read_file(const char* file_name)
{
  int i;

  int offset = 0;

  map_file_dep(file_name, &fdep);

  fdep.nrRules = ntohl(*(int*)&fdep.file_ptr[offset]);
  offset += sizeof(int);

  fdep.rules = (struct ondisk_wordseq_rule*)&fdep.file_ptr[offset];
  offset += sizeof(struct ondisk_wordseq_rule) * fdep.nrRules;
  fdep.nrNodes = ntohl(*(int*)&fdep.file_ptr[offset]);
  offset += sizeof(int);

  fdep.nodes = malloc(sizeof(struct dep_node) * fdep.nrNodes);
  for (i = 0; i < fdep.nrNodes; ++i) {
    read_node(&fdep, &fdep.nodes[i], &offset);
  }
}

int
anthy_get_nr_dep_rule()
{
  return fdep.nrRules;
}

void
anthy_get_nth_dep_rule(int index, struct wordseq_rule *rule)
{
  /* ディスク上の情報からデータを取り出す */
  struct ondisk_wordseq_rule *r = &fdep.rules[index];
  anthy_wtype_set_pos(&rule->wt, r->wt[0]);
  anthy_wtype_set_cos(&rule->wt, r->wt[1]);
  anthy_wtype_set_scos(&rule->wt, r->wt[2]);
  anthy_wtype_set_cc(&rule->wt, r->wt[3]);
  anthy_wtype_set_ct(&rule->wt, r->wt[4]);
  rule->wt.wf = r->wt[5];
  rule->ratio = ntohl(r->ratio);
  rule->node_id = ntohl(r->node_id);
}

int
anthy_init_depword_tab()
{
  const char *fn;

  fn = anthy_conf_get_str("DEPGRAPH");
  if (!fn) {
    anthy_log(0, "Dependent word dictionary is unspecified.\n");
    return -1;
  }
  read_file(fn);
  return 0;
}

void
anthy_release_depword_tab(void)
{
  int i, j;
  for (i = 0; i < fdep.nrNodes; i++) {
    struct dep_node* node = &fdep.nodes[i];
    for (j = 0; j < node->nr_branch; j++) {
      free(node->branch[j].str);
    }
    free(node->branch);
  }
  free(fdep.nodes);

  munmap(fdep.file_ptr, fdep.file_size);
}


/*
 * ʸ��μ�Ω����(��Ƭ�����������ޤ�)��³��
 * ���졢��ư��ʤɤ���°��Υѥ�����򤿤ɤ롣
 * �ѥ�����ϥ���դȤ�������ե�������Ѱդ��롣
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

#include "config.h"
#include <anthy.h>

#include <conf.h>
#include <ruleparser.h>
#include <xstr.h>
#include <filemap.h>
#include <logger.h>
#include <segclass.h>
#include <splitter.h>
#include <wtype.h>
#include <diclib.h>
#include <matrix.h>
#include "wordborder.h"

/* ���ܥ���� */
static struct dep_dic ddic;
static int *dep_matrix;


static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, struct dep_branch *db);
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node);


static int
anthy_xstrcmp_with_ondisk(xstr *xs,
			  ondisk_xstr *dxs)
{
  int *d = (int *)dxs;
  int len = anthy_dic_ntohl(d[0]);
  int i;
  if (len != xs->len) {
    return 1;
  }
  d++;
  for (i = 0; i < len; i++) {
    if (xs->str[i] != d[i]) {
      return 1;
    }
  }
  return 0;
}

static ondisk_xstr *
anthy_next_ondisk_xstr(ondisk_xstr *dxs)
{
  int *d = (int *)dxs;
  int len = anthy_dic_ntohl(d[0]);
  return &d[len+1];
}

static int
anthy_ondisk_xstr_len(ondisk_xstr *dxs)
{
  int *d = (int *)dxs;
  return anthy_dic_ntohl(d[0]);
}

/*
 * �ƥΡ��ɤˤ��������ܾ���ƥ��Ȥ���
 *
 * wl ��Ω������word_list
 * follow_str ��Ω�����ʹߤ�ʸ����
 * node �롼����ֹ�
 */
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node)
{
  struct dep_node *dn = &ddic.nodes[node];
  struct dep_branch *db;
  int i,j;

  /* �ƥ롼��� */
  for (i = 0; i < dn->nr_branch; i++) {
    ondisk_xstr *dep_xs;
    db = &dn->branch[i];
    dep_xs = db->xstrs;

    /* �����ܾ�� */
    for (j = 0; j < db->nr_strs;
	 j++, dep_xs = anthy_next_ondisk_xstr(dep_xs)) {
      xstr cond_xs;
      /* ��°����������ܾ����Ĺ�����Ȥ�ɬ�� */
      if (follow_str.len < anthy_ondisk_xstr_len(dep_xs)) {
	continue;
      }
      /* ���ܾ�����ʬ���ڤ�Ф� */
      cond_xs.str = follow_str.str;
      cond_xs.len = anthy_ondisk_xstr_len(dep_xs);

      /* ���ܾ�����Ӥ��� */
      if (!anthy_xstrcmp_with_ondisk(&cond_xs, dep_xs)) {
	/* ���ܾ���match���� */
	struct word_list new_wl = *wl;
	struct part_info *part = &new_wl.part[PART_DEPWORD];
	xstr new_follow;

	part->len += cond_xs.len;
	new_follow.str = &follow_str.str[cond_xs.len];
	new_follow.len = follow_str.len - cond_xs.len;
	/* ���ܤ��Ƥߤ� */
	match_branch(sc, &new_wl, &new_follow, db);
      }
    }
  }
}

static int
get_row_index_from_wtype(wtype_t wt)
{
  int col = 3;
  int pos, scos;
  pos = anthy_wtype_get_pos(wt);
  scos = anthy_wtype_get_scos(wt);
  if (pos == POS_NOUN) {
    col = 1;
    if (scos == SCOS_T35) {
      col = 4;
    } else if (scos == SCOS_T30) {
      col = 5;
    }
  } else if (pos == POS_V) {
    col = 2;
  }

  return col;
}

static void
calc_dep_score(struct word_list *wl, xstr *xs)
{
  xstr dep;
  dep.len = wl->part[PART_DEPWORD].len;
  dep.str = xs->str - dep.len;
  if (dep.len == 0) {
    wl->dep_score = 2000;
  } else {
    int hash, col;
    col = get_row_index_from_wtype(wl->part[PART_CORE].wt);
    hash = anthy_xstr_hash(&dep);
    wl->dep_score *= (anthy_matrix_image_peek(dep_matrix, col, hash) / 8 +
		      RATIO_BASE);
    wl->dep_score /= RATIO_BASE;
  }
}

/*
 * �����ܤ�¹Ԥ��Ƥߤ�
 *
 * tmpl �����ޤǤ˹�������word_list
 * xs �Ĥ��ʸ����
 * db ����Ĵ�����branch
 */
static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, struct dep_branch *db)
{
  struct part_info *part = &tmpl->part[PART_DEPWORD];
  int i;

  /* ��������˥ȥ饤���� */
  for (i = 0; i < db->nr_transitions; i++) {
    int head_pos = tmpl->head_pos; /* �ʻ�ξ��� */
    enum dep_class dc = part->dc;
    struct dep_transition *transition = &db->transition[i];

    tmpl->tail_ct = anthy_dic_ntohl(transition->ct);
    /* ���ܤγ��ѷ����ʻ� */
    if (anthy_dic_ntohl(transition->dc) != DEP_NONE) {
      part->dc = anthy_dic_ntohl(transition->dc);
    }
    /* ̾�첽����ư�������ʻ�̾���� */
    if (anthy_dic_ntohl(transition->head_pos) != POS_NONE) {
      tmpl->head_pos = anthy_dic_ntohl(transition->head_pos);
    }
    /**/
    tmpl->dep_score *= anthy_dic_ntohl(transition->trans_ratio);
    tmpl->dep_score /= RATIO_BASE;

    /* ���ܤ���ü�� */
    if (anthy_dic_ntohl(transition->next_node)) {
      /* ���� */
      match_nodes(sc, tmpl, *xs, anthy_dic_ntohl(transition->next_node));
    } else {
      struct word_list *wl;

      /* 
       * ��ü�Ρ��ɤ���ã�����Τǡ�
       * �����word_list�Ȥ��ƥ��ߥå�
       */
      wl = anthy_alloc_word_list(sc);
      *wl = *tmpl;
      wl->len += part->len;

      calc_dep_score(wl, xs);
      /**/
      anthy_commit_word_list(sc, wl);
    }
    /* ���ᤷ */
    part->dc = dc;
    tmpl->head_pos = head_pos;
  }
}

/** ��������
 */
void
anthy_scan_node(struct splitter_context *sc,
		struct word_list *tmpl,
		xstr *follow, int node)
{
  /* ��°����դ��Ƥ��ʤ����֤��鸡���򳫻Ϥ��� */
  match_nodes(sc, tmpl, *follow, node);
}




static void
read_xstr(struct dep_dic* ddic, int* offset)
{
  int len = anthy_dic_ntohl(*(int*)&ddic->file_ptr[*offset]);
  *offset += sizeof(int);
  *offset += sizeof(xchar) * len;
}

static void
read_branch(struct dep_dic* ddic, struct dep_branch* branch, int* offset)
{
  int i;

  /* ���ܾ��ο����ɤ� */
  branch->nr_strs = anthy_dic_ntohl(*(int*)&ddic->file_ptr[*offset]);
  *offset += sizeof(int);
  /* ���ܾ���ʸ������ɤ߼�� */
  branch->xstrs = (ondisk_xstr *)&ddic->file_ptr[*offset];

  for (i = 0; i < branch->nr_strs; ++i) {
    read_xstr(ddic, offset);
  }

  branch->nr_transitions = anthy_dic_ntohl(*(int*)&ddic->file_ptr[*offset]);
  *offset += sizeof(int);
  branch->transition = (struct dep_transition*)&ddic->file_ptr[*offset];
  *offset += sizeof(struct dep_transition) * branch->nr_transitions;
}

static void
read_node(struct dep_dic* ddic, struct dep_node* node, int* offset)
{
  int i;
  node->nr_branch = anthy_dic_ntohl(*(int*)&ddic->file_ptr[*offset]);
  *offset += sizeof(int);
    
  node->branch = malloc(sizeof(struct dep_branch) * node->nr_branch);
  for (i = 0; i < node->nr_branch; ++i) {
    read_branch(ddic, &node->branch[i], offset);
  }
}

static void
read_file(void)
{
  int i;

  int offset = 0;

  ddic.file_ptr = anthy_file_dic_get_section("dep_dic");

  /* �ǽ�˥롼��ο� */
  ddic.nrRules = anthy_dic_ntohl(*(int*)&ddic.file_ptr[offset]);
  offset += sizeof(int);

  /* �ƥ롼������ */
  ddic.rules = (struct ondisk_wordseq_rule*)&ddic.file_ptr[offset];
  offset += sizeof(struct ondisk_wordseq_rule) * ddic.nrRules;
  /* �Ρ��ɤο� */
  ddic.nrNodes = anthy_dic_ntohl(*(int*)&ddic.file_ptr[offset]);
  offset += sizeof(int);

  /* �ƥΡ��ɤ��ɤ߹��� */
  ddic.nodes = malloc(sizeof(struct dep_node) * ddic.nrNodes);
  for (i = 0; i < ddic.nrNodes; ++i) {
    read_node(&ddic, &ddic.nodes[i], &offset);
  }
}

int
anthy_get_nr_dep_rule()
{
  return ddic.nrRules;
}

void
anthy_get_nth_dep_rule(int index, struct wordseq_rule *rule)
{
  /* �ǥ�������ξ��󤫤�ǡ�������Ф� */
  struct ondisk_wordseq_rule *r = &ddic.rules[index];
  rule->wt = anthy_get_wtype(r->wt[0], r->wt[1], r->wt[2], r->wt[3], r->wt[4], r->wt[5]);
  rule->node_id = anthy_dic_ntohl(r->node_id);
}

int
anthy_init_depword_tab()
{
  dep_matrix = anthy_file_dic_get_section("matrix");
  read_file();
  return 0;
}

void
anthy_quit_depword_tab(void)
{
  int i;
  for (i = 0; i < ddic.nrNodes; i++) {
    struct dep_node* node = &ddic.nodes[i];
    free(node->branch);
  }
  free(ddic.nodes);
}

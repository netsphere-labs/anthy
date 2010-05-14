/*
 * 品詞型を管理する
 * 中身はwtype_tの内部のレイアウトに強く依存する。
 *
 * Copyright (C) 2000-2004 TABATA Yusuke
 */
#include <stdio.h>
#include <string.h>

#include <wtype.h>
#include "dic_main.h"

wtype_t anthy_wt_none, anthy_wt_all;

/* 品詞の日本語の名前を品詞に変換するテーブル */
static struct PTab {
  const char *name;
  int pos;
  int ct;
  int cc;
  int cos;
  int scos;
  int flags;
} ptab[]= {
#include "ptab.h"
};

/* 辞書中の品詞の名前を品詞に変換するテーブル */
static struct wttable {
  const char *name;
  int cc;
  int pos;
  int cos;
  int scos;
  int ct;/*カ変など*/
  int flags;
} wttab[]= {
#include "wtab.h"
};

static struct PTab *
get_pos_by_name(const char *name)
{
  struct PTab *p;
  for (p = ptab ; p->name ; p++) {
    if (!strcmp(p->name, name)) {
      return p;
    }
  }
  printf("Unknown name of POS %s\n", name);
  return NULL;
}

static struct wttable *
get_table_by_name(const char *s)
{
  struct wttable *w;
  for (w = &wttab[0]; w->name; w++) {
    if (!strcmp(w->name, s)) {
      return w;
    }
  }
  return NULL;
}

void
anthy_init_wtypes(void)
{
  anthy_wt_all.pos = POS_NONE;
  anthy_wt_all.cc = CC_NONE;
  anthy_wt_all.ct = CT_NONE;
  anthy_wt_all.cos = COS_NONE;
  anthy_wt_all.scos = SCOS_NONE;
  anthy_wt_all.wf = WF_NONE;

  anthy_wt_none = anthy_wt_all;
  anthy_wt_none.pos = POS_INVAL;
}

const char *
anthy_type_to_wtype(const char *s, wtype_t *t)
{
  struct wttable *w;
  t->pos = POS_INVAL;
  if (s[0] != '#') {
    return NULL;
  }
  *t = anthy_wt_all;
  w = get_table_by_name(s);
  if (!w) {
    return NULL;
  }
  t->cc = w->cc;
  t->ct = w->ct;
  t->pos = w->pos;
  t->cos = w->cos;
  t->scos = w->scos;
  t->wf = w->flags;
  return w->name;
}

int
anthy_init_wtype_by_name(const char *name, wtype_t *w)
{
  struct PTab *p;
  p = get_pos_by_name(name);
  *w = anthy_wt_all;
  if (p) {
    anthy_wtype_set_pos(w, p->pos);
    anthy_wtype_set_ct(w, p->ct);
    anthy_wtype_set_cc(w, p->cc);
    anthy_wtype_set_cos(w, p->cos);
    anthy_wtype_set_scos(w, p->scos);
    w->wf = p->flags;
    return 0;
  }
  printf("Failed to find wtype(%s).\n", name);
  return -1;
}

void
anthy_print_wtype(wtype_t w)
{
  printf("(POS=%d,COS=%d,SCOS=%d,CC=%d,CT=%d,flags=%d)\n",
	 w.pos, w.cos, w.scos,
	 w.cc, w.ct, w.wf);
}

/* n は hs の一部かどうか？ */
int
anthy_wtypecmp(wtype_t hs, wtype_t n)
{
  /*printf("POS %d,%d\n", hs.type[WT_POS], n.type[WT_POS]);*/
  if (hs.pos != POS_NONE &&
      hs.pos != n.pos) {
    return 0;
  }
  if (hs.cc != CC_NONE &&
      hs.cc != n.cc) {
    return 0;
  }
  if (hs.ct != CT_NONE &&
      hs.ct != n.ct) {
    return 0;
  }
  if (hs.cos != COS_NONE &&
      hs.cos != n.cos) {
    return 0;
  }
  if (hs.scos != SCOS_NONE &&
      hs.scos != n.scos) {
    return 0;
  }
  return 1;
}

const char *
anthy_name_intern(const char *name)
{
  struct PTab *p = get_pos_by_name(name);
  if (p) {
    return p->name;
  }
  return NULL;
}

int
anthy_wtype_get_cc(wtype_t t)
{
  return t.cc;
}

int
anthy_wtype_get_ct(wtype_t t)
{
  return t.ct;
}

int
anthy_wtype_get_pos(wtype_t t)
{
  return t.pos;
}

int
anthy_wtype_get_cos(wtype_t t)
{
  return t.cos;
}

int
anthy_wtype_get_scos(wtype_t t)
{
  return t.scos;
}

int
anthy_wtype_get_indep(wtype_t t)
{
  return t.wf & WF_INDEP;
}

int
anthy_wtype_get_meisi(wtype_t w)
{
  return w.wf & WF_MEISI;
}

int
anthy_wtype_get_sv(wtype_t w)
{
  return w.wf & WF_SV;
}

int
anthy_wtype_get_ajv(wtype_t w)
{
  return w.wf & WF_AJV;
}

void
anthy_wtype_set_cc(wtype_t *w, int cc)
{
  w->cc = cc;
}

void
anthy_wtype_set_ct(wtype_t *w, int ct)
{
  w->ct = ct;
}

void
anthy_wtype_set_pos(wtype_t *w, int pos)
{
  w->pos = pos;
}

void
anthy_wtype_set_cos(wtype_t *w, int cs)
{
  w->cos = cs;
}

void
anthy_wtype_set_scos(wtype_t *w, int sc)
{
  w->scos = sc;
}

void
anthy_wtype_set_dep(wtype_t *w, int isDep)
{
  if (isDep) {
    w->wf &= (~WF_INDEP);
  }else{
    w->wf |= WF_INDEP;
  }
}

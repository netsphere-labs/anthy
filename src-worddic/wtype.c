/*
 * 品詞型を管理する
 * 中身はwtype_tの内部のレイアウトに強く依存する。
 *
 * Copyright (C) 2000-2007 TABATA Yusuke
 */
/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <stdio.h>
#include <string.h>

#include <anthy/wtype.h>
#include "dic_main.h"

wtype_t anthy_wt_none, anthy_wt_all;

wtype_t anthy_wtype_noun, anthy_wtype_num_noun;
wtype_t anthy_wtype_a_tail_of_v_renyou;

struct wttable {
  const char *name;
  int pos;
  int cos;
  int scos;
  int cc;
  int ct;/*カ変など*/
  int flags;
};

/* 辞書中の品詞の名前を品詞に変換するテーブル */
static struct wttable wt_name_tab[]= {
#include "wtab.h"
};

static wtype_t
anthy_get_wtype (int pos, int cos, int scos, int cc, int ct, int wf)
{
  union {
    unsigned int u;
    wtype_t wt;
  } w;

  w.u = 0;

  w.wt.pos = pos;
  w.wt.cos = cos;
  w.wt.scos = scos;
  w.wt.cc = cc;
  w.wt.ct = ct;
  w.wt.wf = wf;

  return w.wt;
}

static struct wttable *
find_wttab(struct wttable *array, const char *name)
{
  struct wttable *w;
  for (w = array; w->name; w++) {
    if (!strcmp(w->name, name)) {
      return w;
    }
  }
  return NULL;
}

void
anthy_init_wtypes(void)
{
  anthy_wt_all = anthy_get_wtype (POS_NONE, COS_NONE, SCOS_NONE,
				  CC_NONE, CT_NONE, WF_NONE);

  anthy_wt_none = anthy_get_wtype (POS_INVAL, COS_NONE, SCOS_NONE,
				   CC_NONE, CT_NONE, WF_NONE);

  /* {"名詞35",POS_NOUN,COS_NONE,SCOS_T35,CC_NONE,CT_NONE,WF_INDEP} */
  anthy_type_to_wtype ("#T", &anthy_wtype_noun);

  /* {"数詞",POS_NUMBER,COS_NN,SCOS_NONE,CC_NONE,CT_NONE,WF_INDEP} */
  anthy_type_to_wtype ("#NN", &anthy_wtype_num_noun); /* exported for ext_ent.c */

  /* {"形容詞化接尾語",POS_D2KY,COS_NONE,SCOS_A1,CC_NONE,CT_HEAD,WF_INDEP} */
  /* {"#D2KY",POS_D2KY,COS_SUFFIX,SCOS_A1,CC_A_KU,CT_HEAD,WF_INDEP} # "形容詞化接尾語(しづらい,がたい)" */
  anthy_type_to_wtype ("#D2KY", &anthy_wtype_a_tail_of_v_renyou); /* exported for metaword.c */
}

/*
 * 返り値には品詞の名前
 * tには品詞が返される
 */
const char *
anthy_type_to_wtype(const char *s, wtype_t *t)
{
  struct wttable *w;
  if (s[0] != '#') {
    *t = anthy_wt_none;
    return NULL;
  }
  w = find_wttab(wt_name_tab, s);
  if (!w) {
    *t = anthy_wt_all;
    return NULL;
  }
  *t = anthy_get_wtype(w->pos, w->cos, w->scos, w->cc, w->ct, w->flags);
  return w->name;
}

/* 二つの品詞が完全に一致しているかどうか */
int
anthy_wtype_equal(wtype_t lhs, wtype_t rhs)
{
  union {
    unsigned int u;
    wtype_t wt;
  } l, r;

  l.wt = lhs;
  r.wt = rhs;

  return (l.u == r.u);
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
anthy_wtype_get_wf(wtype_t t)
{
  return t.wf;
}

int
anthy_wtype_get_indep(wtype_t t)
{
  return t.wf & WF_INDEP;
}

int
anthy_wtype_get_sv(wtype_t w)
{
  return w.wf & WF_SV;
}

void
anthy_print_wtype(wtype_t w)
{
  printf("(POS=%d,COS=%d,SCOS=%d,CC=%d,CT=%d,flags=%d)\n",
	 anthy_wtype_get_pos(w),
	 anthy_wtype_get_cos(w),
	 anthy_wtype_get_scos(w),
	 anthy_wtype_get_cc(w),
	 anthy_wtype_get_ct(w),
	 anthy_wtype_get_wf(w));
}

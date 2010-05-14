/*
 * mem_dic 辞書のキャッシュを行う
 *
 * Copyright (C) 2000-2003 TABATA Yusuke
 */
#include <stdlib.h>

#include <alloc.h>
#include "dic_main.h"
#include "mem_dic.h"

static allocator mem_dic_ator;

static void
dic_ent_dtor(void *p)
{
  struct dic_ent *de = p;
  free(de->str.str);
}

static void
seq_ent_dtor(void *p)
{
  struct seq_ent *seq = p;
  int i;
  /**/
  for (i = 0; i < seq->nr_dic_ents; i++) {
    anthy_sfree(seq->md->dic_ent_allocator, seq->dic_ents[i]);
  }
  if (seq->nr_dic_ents) {
    free(seq->dic_ents);
  }
  /**/
  for (i = 0; i < seq->nr_compound_ents; i++) {
    anthy_sfree(seq->md->compound_ent_allocator, seq->compound_ents[i]);
  }
  if (seq->nr_compound_ents) {
    free(seq->compound_ents);
  }
  /**/
  free(seq->str.str);
}

static void
compound_ent_dtor(void *p)
{
  struct compound_ent *ce = p;
  if (ce->str) {
    anthy_free_xstr(ce->str);
  }
}

static void
mem_dic_dtor(void *p)
{
  struct mem_dic * md = p;
  anthy_free_allocator(md->seq_ent_allocator);
  anthy_free_allocator(md->dic_ent_allocator);
  anthy_free_allocator(md->compound_ent_allocator);
}

/** xstrに対応するseq_entのメモリを確保する */
static struct seq_ent *
alloc_seq_ent_by_xstr(struct mem_dic * md, xstr *x)
{
  struct seq_ent *se;
  int mask = anthy_get_current_session_mask();
  se = (struct seq_ent *)anthy_smalloc(md->seq_ent_allocator);
  se->flags = F_NONE;
  se->seq_type = 0;
  se->md = md;
  se->str.len = x->len;
  /**/
  se->nr_dic_ents = 0;
  se->dic_ents = NULL;
  /**/
  se->nr_compound_ents = 0;
  se->compound_ents = NULL;

  se->str.str = anthy_xstr_dup_str(x);
  se->mask = mask;
  return se;
}

/* ハッシュ関数。とりあえずてきとー */
static int
hash_function(xstr *xs)
{
  if (xs->len) {
    return xs->str[0]% HASH_SIZE;
  }
  return 0;
}

static int
id_hash(int id)
{
  return id % HASH_SIZE;
}

struct dic_ent *
anthy_mem_dic_word_id_to_dic_ent(struct mem_dic *md, int id)
{
  struct dic_ent *de;
  int h = id_hash(id);

  for (de = md->dic_ent_hash[h]; de; de = de->next) {
    if (de->id == id) {
      return de;
    }
  }
  return 0;
}

/** xstrに対応するseq_entを返す */
struct seq_ent *
anthy_mem_dic_alloc_seq_ent_by_xstr(struct mem_dic * md, xstr *xs)
{
  struct seq_ent *se;
  int h;
  /* キャッシュにあればそれを返す */
  se = anthy_mem_dic_find_seq_ent_by_xstr(md, xs);
  if (se) {
    return se;
  }
  /* キャッシュには無いので作る */
  se = alloc_seq_ent_by_xstr(md, xs);

  /* mem_dic中につなぐ */
  h = hash_function(xs);
  se->next = md->seq_ent_hash[h];
  md->seq_ent_hash[h] = se;

  return se;
}

/*** mem_dicの中から文字列に対応するseq_ent*を取得する
 * */
struct seq_ent *
anthy_mem_dic_find_seq_ent_by_xstr(struct mem_dic * md, xstr *xs)
{
  struct seq_ent *sn;
  int h;
  h = hash_function(xs);
  for (sn = md->seq_ent_hash[h]; sn; sn = sn->next) {
    if (!anthy_xstrcmp(&sn->str, xs)){
      int mask;
      mask = anthy_get_current_session_mask();
      sn->mask |= mask;
      return sn;
    }
  }
  return 0;
}

void
anthy_mem_dic_release_seq_ent(struct mem_dic * md, xstr *xs)
{
  struct seq_ent *sn;
  struct seq_ent **sn_prev_p;
  int h;

  h = hash_function(xs);
  sn_prev_p = &md->seq_ent_hash[h];
  for (sn = md->seq_ent_hash[h]; sn; sn = sn->next) {
    if (!anthy_xstrcmp(&sn->str, xs)) {
      *sn_prev_p = sn->next;
      anthy_sfree(md->seq_ent_allocator, sn);
      return;
    } else {
      sn_prev_p = &sn->next;
    }
  }
}

void
anthy_invalidate_seq_ent_mask(struct mem_dic *md, int mask)
{
  int i;
  struct seq_ent *n;
  for (i = 0; i < HASH_SIZE; i++) {
    for (n = md->seq_ent_hash[i]; n; n = n->next) {
      n->mask &= mask;
    }
  }
}

static void
add_dic_ent_to_word_hash(struct mem_dic *md, struct dic_ent *de)
{
  int h = id_hash(de->id);
  de->next = md->dic_ent_hash[h];
  md->dic_ent_hash[h] = de;
}

static void
remove_dic_ent_from_word_hash(struct mem_dic *md, struct dic_ent *de)
{
  int h = id_hash(de->id);
  struct dic_ent *d, **d_prev_p;

  d_prev_p = &md->dic_ent_hash[h];
  for (d = md->dic_ent_hash[h]; d; d = d->next) {
    if (de == d) {
      *d_prev_p = d->next;
      return ;
    }
    d_prev_p = &d->next;
  }
}

/** seq_entにdic_entを追加する */
void
anthy_mem_dic_push_back_dic_ent(struct seq_ent *se, xstr *xs, wtype_t wt,
				const char *wt_name, int freq, int id)
{
  /* 内容を初期化する */
  struct dic_ent *de;
  de = anthy_smalloc(se->md->dic_ent_allocator);
  de->type = wt;
  de->wt_name = wt_name;
  de->freq = freq;
  de->id = id;
  de->str.len = xs->len;
  de->str.str = anthy_xstr_dup_str(xs);
  de->se = se;

  /* 配列に追加する */
  se->nr_dic_ents ++;
  se->dic_ents = realloc(se->dic_ents,
			 sizeof(struct dic_ent *)*se->nr_dic_ents);
  se->dic_ents[se->nr_dic_ents-1] = de;

  /**/
  add_dic_ent_to_word_hash(se->md, de);
}

/** seq_entにcompound_entを追加する */
void
anthy_mem_dic_push_back_compound_ent(struct seq_ent *se, xstr *xs,
				     wtype_t wt, int freq)
{
  struct compound_ent *ce;
  ce = anthy_smalloc(se->md->compound_ent_allocator);
  ce->type = wt;
  ce->str = xs;
  ce->freq = freq;

  se->nr_compound_ents ++;
  se->compound_ents = realloc(se->compound_ents,
			      sizeof(struct compound_ent *) *
			      se->nr_compound_ents);
  se->compound_ents[se->nr_compound_ents-1] = ce;
}

struct mem_dic *
anthy_create_mem_dic(void)
{
  int i;
  struct mem_dic *md;

  md = anthy_smalloc(mem_dic_ator);
  for (i = 0; i < HASH_SIZE; i++) {
    md->seq_ent_hash[i] = NULL;
    md->dic_ent_hash[i] = NULL;
  }
  
  md->seq_ent_allocator = 
    anthy_create_allocator(sizeof(struct seq_ent),
			   seq_ent_dtor);
  md->dic_ent_allocator =
    anthy_create_allocator(sizeof(struct dic_ent),
			   dic_ent_dtor);
  md->compound_ent_allocator =
    anthy_create_allocator(sizeof(struct compound_ent),
			   compound_ent_dtor);
  anthy_init_sessions(md);

  return md;
}

void
anthy_release_mem_dic(struct mem_dic * d)
{
  anthy_sfree(mem_dic_ator, d);
}

void
anthy_shrink_mem_dic(struct mem_dic * md)
{
  int i;
  struct seq_ent *n, *n_next;
  struct seq_ent **n_prev_p;

  for ( i = 0 ; i < HASH_SIZE ; i ++){
    n_prev_p = &md->seq_ent_hash[i];
    for (n = md->seq_ent_hash[i]; n; n = n_next) {
      n_next = n->next;
      if (!n->mask) {
	/*どのセッションによっても使われていない*/
	int i;
	for (i = 0; i < n->nr_dic_ents; i++) {
	  remove_dic_ent_from_word_hash(md, n->dic_ents[i]);
	}
	*n_prev_p = n_next;
	anthy_sfree(md->seq_ent_allocator, n);
      } else
	n_prev_p = &n->next;
    }
  }
}

void
anthy_init_mem_dic(void)
{
  mem_dic_ator = anthy_create_allocator(sizeof(struct mem_dic),
					mem_dic_dtor);
}

void
anthy_quit_mem_dic(void)
{
  anthy_free_allocator(mem_dic_ator);
}

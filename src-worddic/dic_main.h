#ifndef _dic_main_h_included_
#define _dic_main_h_included_

#include <dic.h>
#include <word_dic.h>
#include <wtype.h>
#include <xstr.h>


/* 辞書中の頻度に対して内部の頻度の倍率 */
#define FREQ_RATIO 4


/* dic_main.c */
int anthy_init_dic_cache(void);
struct seq_ent *anthy_cache_get_seq_ent(xstr *x, int is_reverse);




/* word_dic.c */
void anthy_init_word_dic(void);
struct word_dic* anthy_create_word_dic(void);
void anthy_release_word_dic(struct word_dic *);
char *anthy_word_dic_get_hashmap_ptr(struct word_dic *);
void anthy_word_dic_fill_seq_ent_by_xstr(struct word_dic *, xstr *,
					 struct seq_ent *, int);
void anthy_fill_dic_ent(char *dic ,int idx, struct seq_ent *seq, 
			xstr* yomi, int is_reverse);


/* use_dic.c */
void anthy_init_use_dic(void);
void anthy_quit_use_dic(void);
int anthy_word_dic_check_word_relation(struct word_dic *,
				       int from, int to);
/* dic_session.h */
/*32 bitのマスクを使う。*/
#define MAX_SESSION 32
struct dic_session {
  /* 0〜(MAX_SESSION-1)までの番号 */
  int id;
  /* マスク(1<<id) */
  int mask;
  /* 使用中かどうかのフラグ */
  int is_free;
  struct mem_dic *dic;
};
void anthy_init_sessions(struct mem_dic *d);
struct dic_session *anthy_create_session(void);
void anthy_activate_session(struct dic_session *);
void anthy_release_session(struct dic_session *);
int anthy_get_current_session_mask(void);

/* mem_dic.c */
void anthy_init_mem_dic(void);
void anthy_quit_mem_dic(void);
struct mem_dic * anthy_create_mem_dic(void);
void anthy_release_mem_dic(struct mem_dic * );
/* node がなければ作る */
struct seq_ent *anthy_mem_dic_alloc_seq_ent_by_xstr(struct mem_dic * d,
						    xstr *, int is_reverse);
/* node がなければ作らない */
struct seq_ent *anthy_mem_dic_find_seq_ent_by_xstr(struct mem_dic * d,
						   xstr *, int is_reverse);
/**/
void anthy_mem_dic_push_back_dic_ent(struct seq_ent *, xstr *,
				     wtype_t wt, const char *wt_name,
				     int freq);
void anthy_mem_dic_push_back_compound_ent(struct seq_ent *, xstr *,
					  wtype_t , int freq);
void anthy_mem_dic_release_seq_ent(struct mem_dic * d, xstr *, int is_release);
void anthy_shrink_mem_dic(struct mem_dic * d);


/* priv_dic.c */
void anthy_init_private_dic(const char *id);
void anthy_copy_words_from_private_dic(struct seq_ent *seq, xstr *xs,
				       int is_reverse);
void anthy_release_private_dic(void);
void anthy_check_user_dir(void);
void anthy_priv_dic_lock(void);
void anthy_priv_dic_unlock(void);
void anthy_priv_dic_update(void);


#endif

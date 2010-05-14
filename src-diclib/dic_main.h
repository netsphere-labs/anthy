#ifndef _dic_main_h_included_
#define _dic_main_h_included_

#include <dic.h>


/* 辞書中の頻度に対して内部の頻度の倍率 */
#define FREQ_RATIO 4


/* dic_session.h */
/*32 bitのマスクを使う。*/
#define MAX_SESSION 32
struct dic_session {
  int id;
  int mask;
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
struct seq_ent *anthy_mem_dic_alloc_seq_ent_by_xstr(struct mem_dic * d,xstr *);
/* node がなければ作らない */
struct seq_ent *anthy_mem_dic_find_seq_ent_by_xstr(struct mem_dic * d,xstr *);
void anthy_mem_dic_push_back_dic_ent(struct seq_ent *,xstr *,wtype_t ,
				     const char *wt_name, int freq, int id);
void anthy_mem_dic_push_back_compound_ent(struct seq_ent *,xstr *,wtype_t );
struct dic_ent *anthy_mem_dic_word_id_to_dic_ent(struct mem_dic *, int );
void anthy_mem_dic_release_seq_ent(struct mem_dic * d,xstr *);
void anthy_shrink_mem_dic(struct mem_dic * d);



/* dic_main.c */
int anthy_init_dic_cache(void);
struct seq_ent *anthy_cache_get_seq_ent(xstr *x);



/* file_dic.c */
void anthy_init_file_dic(void);
struct file_dic * anthy_create_file_dic(const char *fn);
void anthy_release_file_dic(struct file_dic *);
void anthy_file_dic_fill_seq_ent_by_xstr(struct file_dic *, xstr *,
					 struct seq_ent *);
char *anthy_file_dic_get_hashmap_ptr(struct file_dic *);


/* use_dic.c */
void anthy_init_use_dic(void);
void anthy_quit_use_dic(void);
int anthy_file_dic_check_word_relation(struct file_dic *,
				       int from, int to);


/* wtype.c */
void anthy_init_wtypes(void);



/* xchar.c */
void anthy_init_xchar_tab(void);



/* hash_map.c */
void anthy_init_hashmap(struct file_dic *);
void anthy_quit_hashmap(void);

#endif

/*
 * Interface for personal dictionary
 */
#ifndef _dicutil_h_included_
#define _dicutil_h_included_

/* 返り値 / anthy_priv_dic_add_entry*/
/* OK / 単語が登録できた */
#define ANTHY_DIC_UTIL_OK 0
/* 失敗 / 登録に失敗した */
#define ANTHY_DIC_UTIL_ERROR -1
/* 同じ単語が登録してあった、頻度だけを上書き */
#define ANTHY_DIC_UTIL_DUPLICATE -2

void anthy_dic_util_init(void);
void anthy_dic_util_quit(void);
void anthy_dic_util_set_personality(const char *);
const char *anthy_dic_util_get_anthydir(void);
#define HAS_ANTHY_DICUTIL_SET_ENCODING
int anthy_dic_util_set_encoding(int );

void anthy_priv_dic_delete(void);
int anthy_priv_dic_select_first_entry(void);
int anthy_priv_dic_select_next_entry(void);
int anthy_priv_dic_select_entry(const char *);/* not implemented */

char *anthy_priv_dic_get_index(char *buf, int len);
int anthy_priv_dic_get_freq(void);
char *anthy_priv_dic_get_wtype(char *buf, int len);
char *anthy_priv_dic_get_word(char *buf, int len);

int anthy_priv_dic_add_entry(const char *yomi, const char *word,
			     const char *wt, int freq);

/* experimental and unstable /usr/share/dict/wordsから単語を探す */
#define HAS_ANTHY_DIC_SEARCH_WORDS_FILE
char *anthy_dic_search_words_file(const char *word);

#endif

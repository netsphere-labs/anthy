/* Anthyの汎用のhash */
#ifndef _hash_map_h_included_
#define _hash_map_h_included_

/* 検索用の関数 */
int anthy_check_word_relation(int from, int to);
/* hashmap構築用の関数 */
int anthy_word_relation_hash(int from, int to);

#endif

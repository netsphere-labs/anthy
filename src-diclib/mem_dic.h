#ifndef _mem_dic_h_included_
#define _mem_dic_h_included_

#include "dic_ent.h"


#define HASH_SIZE 64 /*ハッシュテーブルのサイズ…64(なんとなく)*/

/** メモリ辞書 */
struct mem_dic {
  struct seq_ent *seq_ent_hash[HASH_SIZE];
  struct dic_ent *dic_ent_hash[HASH_SIZE];
  struct dic_session sessions[MAX_SESSION];
  allocator seq_ent_allocator;
  allocator dic_ent_allocator;
  allocator compound_ent_allocator;
};

/*マスクと論理積を取る*/
void anthy_invalidate_seq_ent_mask(struct mem_dic * ,int mask);

#endif

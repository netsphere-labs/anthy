#ifndef __diclib_h_included__
#define __diclib_h_included__

#include <stdint.h>

/* 全体の初期化、解放 */
int anthy_init_diclib(void);
void anthy_quit_diclib(void);

void* anthy_file_dic_get_section(const char* section_name);

/*
  utility
 */
uint32_t anthy_dic_ntohl(uint32_t a);
uint32_t anthy_dic_htonl(uint32_t a);


#endif

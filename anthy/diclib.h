#ifndef __diclib_h_included__
#define __diclib_h_included__


/* 全体の初期化、解放 */
int anthy_init_diclib(void);
void anthy_quit_diclib(void);

void* anthy_file_dic_get_section(const char* section_name);

/*
  utility
 */
unsigned int anthy_dic_ntohl(unsigned int a);
unsigned int anthy_dic_htonl(unsigned int a);


#endif

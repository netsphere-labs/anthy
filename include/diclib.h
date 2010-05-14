#ifndef __diclib_h_included__
#define __diclib_h_included__


/* 全体の初期化、解放 */
int anthy_init_diclib(void);
void anthy_quit_diclib(void);

void* anthy_file_dic_get_section(const char* section_name);

/*
  utility
 */
int anthy_dic_ntohl(int a);
int anthy_dic_htonl(int a);


#endif


#ifndef _WIN32
  #include <arpa/inet.h>
#else
  #define STRICT 1
  #include <winsock2.h>
#endif

/**/
#include <anthy/diclib.h>
#include <anthy/xstr.h>
#include <anthy/alloc.h>
#include <anthy/conf.h>
#include "diclib_inner.h"


uint32_t
anthy_dic_ntohl(uint32_t a)
{
  return ntohl(a);
}

uint32_t
anthy_dic_htonl(uint32_t a)
{
  return htonl(a);
}

int
anthy_init_diclib()
{
  anthy_do_conf_init();
  if (anthy_init_file_dic() == -1) {
    return -1;
  }
  anthy_init_xchar_tab();
  anthy_init_xstr();
  return 0;
}

void
anthy_quit_diclib()
{
  anthy_quit_allocator();
  anthy_quit_xstr();
  anthy_conf_free();
}

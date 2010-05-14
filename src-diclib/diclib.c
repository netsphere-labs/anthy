#include <sys/types.h>
#include <netinet/in.h>

/**/
#include <diclib.h>
#include <xstr.h>
#include <alloc.h>
#include <conf.h>
#include "diclib_inner.h"


int
anthy_dic_ntohl(int a)
{
  return ntohl(a);
}

int
anthy_dic_htonl(int a)
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

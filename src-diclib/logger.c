/*
 * ログの出力
 * $Id: logger.c,v 1.8 2002/05/14 13:24:47 yusuke Exp $
 */
#include <stdio.h>
#include <stdarg.h>

#include <anthy/anthy.h>
#include <anthy/logger.h>

static void (*logger)(int lv, const char *str);
static int current_level = 1;

void
anthy_do_set_logger(void (*fn)(int lv, const char *str), int lv)
{
  current_level = lv;
  logger = fn;
  /* to be implemented */
}

static void
do_log(int lv, const char *str, va_list arg)
{
  if (lv < current_level) {
    return ;
  }
  fprintf(stderr, "Anthy: ");
  vfprintf(stderr, str, arg);
}

void
anthy_log(int lv, const char *str, ...)
{
  va_list arg;
  if (lv > current_level) {
    return ;
  }
  va_start(arg, str);
  do_log(lv, str, arg);
  va_end(arg);
}

void
anthy_set_logger(anthy_logger lg, int level)
{
  anthy_do_set_logger(lg, level);
}

/*
 * Anthyの設定のデータベース
 * conf_initで設定される変数とconf_overrideで設定される
 * 変数の関係に注意
 *
 * Copyright (C) 2000-2007 TABATA Yusuke
 */
/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#define _CRT_SECURE_NO_WARNINGS

#ifndef _MSC_VER
  #include <config.h>
#else
  #include <defines.h>
#endif

#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifndef _WIN32
  #include <unistd.h>
  #include <pwd.h>
#else
  #define STRICT 1
  #include <winsock2.h> // gethostname()
  #include <shlobj.h>
  #define strdup _strdup
#endif

#include <anthy/alloc.h>
#include <anthy/conf.h>
#include <anthy/logger.h>


/** 設定の変数と値 */
struct val_ent {
  /** 変数名 */
  const char *var;
  /** 値 */
  const char *val;
  /** リストの次要素 */
  struct val_ent *next;
};

static struct val_ent *ent_list;
/** 初期化済みフラグ */
static int confIsInit;
static allocator val_ent_ator;

static void
val_ent_dtor(void *p)
{
  struct val_ent *v = (struct val_ent*) p;
  free((void *)v->var);
  if (v->val) {
    free((void *)v->val);
  }
}

/** 変数名に対応するval_entを取得する */
static struct val_ent *
find_val_ent(const char *v)
{
  struct val_ent *e;
  for (e = ent_list; e; e = e->next) {
    if(!strcmp(v, e->var)) {
      return e;
    }
  }
  e = (struct val_ent*) malloc(sizeof(struct val_ent));
  if (!e)
    return NULL;

  e->var = strdup(v);
  e->val = 0;
  e->next = ent_list;
  ent_list = e;
  return e;
}

/** ${変数名}の形の変数の値を取得する
 */
static const char *
get_subst(const char *s)
{
  if (s[0] == '$' && s[1] == '{' &&
      strchr(s, '}')) {
    struct val_ent *val;
    char *var = strdup(&s[2]);
    char *k = strchr(var, '}');
    *k = 0;
    val = find_val_ent(var);
    free(var);
    if (!val || !val->val) {
      return "";
    }
    return val->val;
  }
  return NULL;
}

struct expand_buf {
  int len;
  int size;
  char *buf;
  char *cur;
};

static void
ensure_buffer(struct expand_buf *eb, int count)
{
  int required = count - (eb->size - eb->len) + 16;
  if (required > 0) {
    eb->size += required;
    eb->buf = (char*) realloc(eb->buf, eb->size);
    eb->cur = &eb->buf[eb->len];
  }
}

static char *
expand_string(const char *s)
{
  struct expand_buf eb;
  char *res;
  eb.size = 256;
  eb.buf = (char*) malloc(eb.size);
  eb.cur = eb.buf;
  eb.len = 0;

  while (*s) {
    const char *subst = get_subst(s);
    if (subst) {
      int len = strlen(subst);
      ensure_buffer(&eb, len + 1);
      *eb.cur = 0;
      strcat(eb.buf, subst);
      eb.cur += len;
      eb.len += len;
      s = strchr(s, '}');
      s ++;
    } else {
      *eb.cur = *s;
      /**/
      eb.cur ++;
      s++;
      eb.len ++;
    }
    /**/
    ensure_buffer(&eb, 256);
  }
  *eb.cur = 0;
  /**/
  res = strdup(eb.buf);
  free(eb.buf);
  return res;
}

static void
add_val(const char *var, const char *val)
{
  struct val_ent *e;
  e = find_val_ent(var);
  if (e->val) {
    free((void *)e->val);
  }
  e->val = expand_string(val);
}

static void
read_conf_file(void)
{
  const char *fn;
  FILE *fp;
  char buf[1024];
  fn = anthy_conf_get_str("CONFFILE");
  printf("CONFFILE = %s\n", fn);
  fp = fopen(fn, "r"); // text mode.
  if (!fp){
    anthy_log(0, "Failed to open %s\n", fn);
    return ;
  }
  while(fgets(buf, 1024, fp)) {
    if (buf[0] != '#') {
      char var[1024], val[1024];
      if (sscanf(buf, "%s %s", var, val) == 2){
	add_val(var, val);
      }
    }
  }
  fclose(fp);
}

void
anthy_do_conf_override(const char *var, const char *val)
{
  if (!strcmp(var,"CONFFILE")) {
    add_val(var, val);
    anthy_do_conf_init();
  }else{
    anthy_do_conf_init();
    add_val(var, val);
  }
}

/* ユニークなセッションIDを確保する */
#define SID_FORMAT	"%s-%08x-%05d" /* HOST-TIME-PID */
#define MAX_SID_LEN  	(MAX_HOSTNAME+8+5+2)
#define MAX_HOSTNAME 	64

static void
alloc_session_id(void)
{
  time_t t;
#ifndef _WIN32
  pid_t pid;
#else
  DWORD pid;
#endif
  char hn[MAX_HOSTNAME];
  char sid[MAX_SID_LEN];
  t = time(0);
#ifndef _WIN32
  pid = getpid();
#else
  pid = GetCurrentProcessId();
#endif
  gethostname(hn, MAX_HOSTNAME);
  hn[MAX_HOSTNAME-1] = '\0';
  sprintf(sid, SID_FORMAT, hn, (unsigned int) (t & 0xffffffff), pid & 0xffff);
  add_val("SESSION-ID", sid);
}

void
anthy_do_conf_init(void)
{
  const char *fn;
#ifndef _WIN32
  struct passwd *pw;
#else
  char pw_dir[MAX_PATH + 1];
#endif

  if (confIsInit)
    return;

    val_ent_ator = anthy_create_allocator(sizeof(struct val_ent), val_ent_dtor);
    /*デフォルトの値を設定する。*/
    add_val("VERSION", VERSION);
    fn = anthy_conf_get_str("CONFFILE");
    if (!fn){
      add_val("CONFFILE", CONF_DIR"/anthy-conf");
    }
#ifndef _WIN32
    pw = getpwuid(getuid());
    add_val("HOME", pw->pw_dir);
#else
  SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, pw_dir);
  add_val("HOME", pw_dir);
#endif
    alloc_session_id();
    read_conf_file();
    confIsInit = 1;
}

void
anthy_conf_free(void)
{
  struct val_ent *e, *next;
  for (e = ent_list; e; e = next) {
    free((char *)e->var);
    free((char *)e->val);
    next = e->next;
    free(e);
  }
  ent_list = NULL;
  confIsInit = 0;
}

const char *
anthy_conf_get_str(const char *var)
{
  struct val_ent *e;
  e = find_val_ent(var);
  assert(e);
  return e->val;
}

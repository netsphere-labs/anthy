/*
 * 個人辞書を扱うためのコード
 *
 * ユーザが明示的に登録した単語だけでなく、
 * 未知語を自動的に学習して管理するAPIも持つ。
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

#define NO_OLDNAMES 1  // mingw
#define _CRT_SECURE_NO_WARNINGS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32
  #include <dirent.h>
  #include <unistd.h>
#else
  #define STRICT 1
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <direct.h> // _mkdir()
  #ifdef _MSC_VER
    #include <malloc.h> // alloca()
  #endif
  #define off_t _off_t
  #define stat _stat
  #define S_IFREG _S_IFREG
  #define S_IFDIR _S_IFDIR
  #define alloca _alloca
#endif

#include <anthy/anthy.h>
#include <anthy/alloc.h>
#include <anthy/dic.h>
#include <anthy/record.h>
#include <anthy/dicutil.h>
#include <anthy/conf.h>
#include <anthy/logger.h>
#include <anthy/textdic.h>
#include <anthy/word_dic.h>
#include "dic_main.h"
#include "dic_ent.h"

/* 個人辞書 */
char* anthy_private_text_dic;
static char* anthy_imported_text_dic;
static char *imported_dic_dir;

/* File name for dictionary lock.
 * "HOME/.anthy/lock-file_%s"
 */
static char* lock_fn = NULL;

static long lock_depth = 0;

#ifndef _WIN32
static int lock_fd = -1;
#else
static HANDLE lock_fd = INVALID_HANDLE_VALUE;
#endif

#define MAX_DICT_SIZE 100000000

/**
 * Check if HOME/.anthy exists, create the directory if not.
 */
void
anthy_check_user_dir(void)
{
  const char *hd;
  char *dn;
  struct stat st;

  hd = anthy_conf_get_str("HOME");
  dn = alloca(strlen(hd) + 10);
  sprintf(dn, "%s/.anthy", hd);
  if (!stat(dn, &st)) {
    if (!(st.st_mode & S_IFDIR)) {
      anthy_log(0, "Failed to create directory: the HOME/.anthy file exists.\n");
      return; // failed
    }
  }
  else {
    int r;
    /*fprintf(stderr, "Anthy: Failed to open anthy directory(%s).\n", dn);*/
#ifndef _WIN32
    r = mkdir(dn, S_IRWXU);
#else
    r = _mkdir(dn);
#endif
    if (r == -1){
      anthy_log(0, "Failed to create profile directory\n");
      return ; // failed
    }
  }
    /*fprintf(stderr, "Anthy: Created\n");*/
#ifndef _WIN32
    r = chmod(dn, S_IRUSR | S_IWUSR | S_IXUSR);
    if (r == -1) {
      anthy_log(0, "But failed to change permission.\n");
    }
#endif
}

static void
init_lock_fn(const char *home, const char *id)
{
  lock_fn = malloc(strlen(home) + strlen(id) + 40);
  sprintf(lock_fn, "%s/.anthy/lock-file_%s", home, id);
}

static const char *
textdicname (const char *home, const char *name, const char *id)
{
  char *fn = malloc(strlen(home) + strlen(name) + strlen(id) + 10);
  sprintf(fn, "%s/.anthy/%s%s", home, name, id);
  return (const char *)fn;
}

void
anthy_priv_dic_lock(void)
{
#ifndef _WIN32
  struct flock lck;
#endif

#ifndef _WIN32
  if (__sync_add_and_fetch(&lock_depth, 1) > 1)
    return; // succeeded
#else
  if (InterlockedIncrement(&lock_depth) > 1)
    return; // succeeded
#endif

  if (!lock_fn) {
    /* 初期化をミスってる */
#ifndef _WIN32
    lock_fd = -1;
#else
    lock_fd = INVALID_HANDLE_VALUE;
#endif
    return ; // failed
  }

#ifndef _WIN32
  /* ファイルロックの方法は多数あるが、この方法はcygwinでも動くので採用した */
  lock_fd = open(lock_fn, O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
  if (lock_fd == -1) {
    return ;
  }

  lck.l_type = F_WRLCK;
  lck.l_whence = (short) 0;
  lck.l_start = (off_t) 0;
  lck.l_len = (off_t) 1;
  if (fcntl(lock_fd, F_SETLKW, &lck) == -1) {
    close(lock_fd);
    lock_fd = -1;
  }
#else
  lock_fd = CreateFileA(lock_fn, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,  // lpSecurityAttributes
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
  if (lock_fd == INVALID_HANDLE_VALUE)
    return; // failed

  if ( !LockFile(lock_fd, 0, 0, 1, 0) ) {
    CloseHandle(lock_fd);
    lock_fd = INVALID_HANDLE_VALUE;
  }
#endif
}

void
anthy_priv_dic_unlock(void)
{
  lock_depth --;
  if (lock_depth > 0) {
    return ;
  }

#ifndef _WIN32
  if (lock_fd != -1) {
    close(lock_fd);
    lock_fd = -1;
  }
#else
  if (lock_fd != INVALID_HANDLE_VALUE) {
    CloseHandle(lock_fd);
    lock_fd = INVALID_HANDLE_VALUE;
  }
#endif
}

#if 0
/* seq_entに追加する */
static void
add_to_seq_ent(const char *line, int encoding, struct seq_ent *seq)
{
  struct word_line wl;
  wtype_t wt;
  xstr *xs;
  /* */
  if (anthy_parse_word_line(line, &wl)) {
    return ;
  }
  xs = anthy_cstr_to_xstr(wl.word, encoding);
  anthy_type_to_wtype(wl.wt, &wt);
  anthy_mem_dic_push_back_dic_ent(seq, 0, xs, wt,
				  NULL, wl.freq, 0);
  anthy_free_xstr(xs);
}
#endif

void
anthy_copy_words_from_private_dic(struct seq_ent *seq,
				  xstr *xs, int is_reverse)
{
  if (is_reverse) {
    return ;
  }
  /**/
  if (!anthy_select_section("UNKNOWN_WORD", 0) &&
      !anthy_select_row(xs, 0)) {
    wtype_t wt;
    xstr *word_xs;
    anthy_type_to_wtype("#T35", &wt);
    word_xs = anthy_get_nth_xstr(0);
    anthy_mem_dic_push_back_dic_ent(seq, 0, word_xs, wt, NULL, 10, 0);
  }
}

int
anthy_parse_word_line(const char *line, struct word_line *res)
{
  int i;
  const char *buf = line;
  /* default values */
  res->wt[0] = 0;
  res->freq = 1;
  res->word = NULL;
  /* 品詞と頻度をparse */
  for (i = 0; i < 9 && *buf && *buf != '*' && *buf != ' '; buf++, i++) {
    res->wt[i] = *buf;
  }
  res->wt[i] = 0;
  if (*buf == '*') {
    buf ++;
    sscanf(buf, "%d", &res->freq);
    buf = strchr(buf, ' ');
  } else {
    res->freq = 1;
  }
  if (!buf || !(*buf)) {
    res->word = "";
    return -1;
  }
  buf++;
  /* 単語 */
  res->word = buf;
  return 0;
}


/**
 * Pass dictionary file names to a callback function.
 *   - anthy_private_text_dic
 *   - anthy_imported_text_dic
 *   - all of imported_dic_dir
 * @param request_scan  Callback function. parameters are filename and arg.
 */
void
anthy_ask_scan (void (*request_scan)(const char *, void *), void *arg)
{
#ifndef _WIN32
  DIR *dir;
  struct dirent *de;
#else
  HANDLE hFind;
  WIN32_FIND_DATAA find_data;
#endif
  int size = 0;

  request_scan (anthy_private_text_dic, arg);
  request_scan (anthy_imported_text_dic, arg);

#ifndef _WIN32
  dir = opendir(imported_dic_dir);
  if (!dir)
    return ; // failed

  while ((de = readdir(dir))) {
    struct stat st_buf;
    char* fn = malloc(strlen(imported_dic_dir) + strlen(de->d_name) + 3);
    if (!fn)
      break;

    sprintf(fn, "%s/%s", imported_dic_dir, de->d_name);
    if (stat(fn, &st_buf) || !S_ISREG(st_buf.st_mode)) {
      free(fn);
      continue;
    }

    size += st_buf.st_size;
    if (size > MAX_DICT_SIZE) {
      free(fn);
      break;
    }
    request_scan(fn, arg);
    free(fn);
  }
  closedir(dir);
#else
  char* dirname = malloc(strlen(imported_dic_dir) + 3);
  sprintf(dirname, "%s\\*", imported_dic_dir);
  hFind = FindFirstFileA(dirname, &find_data);
  if (hFind == INVALID_HANDLE_VALUE)
    return; // failed
  free(dirname);

  do {
    struct stat st_buf;
    if ( stat(find_data.cFileName, &st_buf) || !(st_buf.st_mode & S_IFREG) )
      continue;

    size += st_buf.st_size;
    if (size > MAX_DICT_SIZE)
      break;
    request_scan(find_data.cFileName, arg);
  } while (FindNextFileA(hFind, &find_data) != 0);
  FindClose(hFind);
#endif
}

static void
add_unknown_word(xstr *yomi, xstr *word)
{
  /* recordに追加 */
  if (anthy_select_section("UNKNOWN_WORD", 1)) {
    return ;
  }
  if (!anthy_select_row(yomi, 0)) {
    anthy_mark_row_used();
  }
  if (anthy_select_row(yomi, 1)) {
    return ;
  }
  anthy_set_nth_xstr(0, word);
}

void
anthy_add_unknown_word(xstr *yomi, xstr *word)
{
  if (!(anthy_get_xstr_type(word) & XCT_KATA) &&
      !(anthy_get_xstr_type(word) & XCT_HIRA)) {
    return ;
  }
  if (yomi->len < 4 || yomi->len > 30) {
    return ;
  }
  /**/
  add_unknown_word(yomi, word);
}

void
anthy_forget_unused_unknown_word(xstr *xs)
{
  /* recordに記録された物を消す */
  if (anthy_select_section("UNKNOWN_WORD", 0)) {
    return ;
  }
  if (!anthy_select_row(xs, 0)) {
    anthy_release_row();
  }
}

void
anthy_init_private_dic(const char *id)
{
  const char *home = anthy_conf_get_str("HOME");
  if (lock_fn) {
    free(lock_fn);
  }
  init_lock_fn(home, id);
  /**/
  anthy_private_text_dic = textdicname (home, "private_words_", id);
  anthy_imported_text_dic = textdicname (home, "imported_words_", id);
  imported_dic_dir = malloc(strlen(home) + strlen(id) + 30);
  sprintf(imported_dic_dir, "%s/.anthy/imported_words_%s.d/", home, id);
}

void
anthy_release_private_dic(void)
{
  free (anthy_private_text_dic);
  free (anthy_imported_text_dic);
  free(imported_dic_dir);
  anthy_private_text_dic = NULL;
  anthy_imported_text_dic = NULL;
  imported_dic_dir = NULL;
  /**/
  if (lock_depth > 0) {
    /* not sane situation */
    lock_depth = 0;
    if (lock_fn) {
#ifndef _WIN32
      unlink(lock_fn);
#else
      DeleteFileA(lock_fn);
#endif
    }
  }
  /**/
  free(lock_fn);
  lock_fn = NULL;
}

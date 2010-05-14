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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <anthy.h>
#include <alloc.h>
#include <dic.h>
#include <record.h>
#include <dicutil.h>
#include <conf.h>
#include <logger.h>
#include <texttrie.h>
#include <textdict.h>
#include <dic_ent.h>
#include <word_dic.h>
#include "dic_main.h"

/* 個人辞書 */
struct text_trie *anthy_private_tt_dic;
struct textdict *anthy_private_text_dic;
/* ロック用の変数 */
static char *lock_fn;
static int lock_depth;
static int lock_fd;

/* 個人辞書のディレクトリの有無を確認する */
void
anthy_check_user_dir(void)
{
  const char *hd;
  char *dn;
  struct stat st;
  hd = anthy_conf_get_str("HOME");
  dn = alloca(strlen(hd) + 10);
  sprintf(dn, "%s/.anthy", hd);
  if (stat(dn, &st) || !S_ISDIR(st.st_mode)) {
    int r;
    /*fprintf(stderr, "Anthy: Failed to open anthy directory(%s).\n", dn);*/
    r = mkdir(dn, S_IRWXU);
    if (r == -1){
      anthy_log(0, "Failed to create profile directory\n");
      return ;
    }
    /*fprintf(stderr, "Anthy: Created\n");*/
    r = chmod(dn, S_IRUSR | S_IWUSR | S_IXUSR);
    if (r == -1) {
      anthy_log(0, "But failed to change permission.\n");
    }
  }
}

static void
init_lock_fn(const char *home, const char *id)
{
  lock_fn = malloc(strlen(home) + strlen(id) + 40);
  sprintf(lock_fn, "%s/.anthy/lock-file_%s", home, id);
}

static struct text_trie *
open_tt_dic(const char *home, const char *id)
{
  struct text_trie *tt;
  char *buf = malloc(strlen(home) + strlen(id) + 40);
  sprintf(buf, "%s/.anthy/private_dict_%s.tt", home, id);
  tt = anthy_trie_open(buf, 1);
  free(buf);
  return tt;
}

static struct textdict *
open_textdic(const char *home, const char *id)
{
  char *fn = malloc(strlen(home) + strlen(id) + 40);
  struct textdict *td;
  sprintf(fn, "%s/.anthy/private_words_%s", home, id);
  td = anthy_textdict_open(fn, 0);
  free(fn);
  return td;
}

void
anthy_priv_dic_lock(void)
{
  struct flock lck;
  lock_depth ++;
  if (lock_depth > 1) {
    return ;
  }
  if (!lock_fn) {
    /* 初期化をミスってる */
    lock_fd = -1;
    return ;
  }

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
}

void
anthy_priv_dic_unlock(void)
{
  lock_depth --;
  if (lock_depth > 0) {
    return ;
  }

  if (lock_fd != -1) {
    close(lock_fd);
    lock_fd = -1;
  }
}

void
anthy_priv_dic_update(void)
{
  anthy_trie_update_mapping(anthy_private_tt_dic);
}

/* seq_entに追加する */
static void
add_to_seq_ent(const char *line, int encoding, struct seq_ent *seq)
{
  struct word_line wl;
  wtype_t wt;
  xstr *xs;
  /* */
  anthy_parse_word_line(line, &wl);
  xs = anthy_cstr_to_xstr(wl.word, encoding);
  anthy_type_to_wtype(wl.wt, &wt);
  anthy_mem_dic_push_back_dic_ent(seq, 0, xs, wt,
				  NULL, wl.freq, 0);
  anthy_free_xstr(xs);
}

/* texttrieに登録されているかをチェックし、
 * 登録されていればseq_entに追加する
 */
static void
copy_words_from_tt(struct seq_ent *seq, xstr *xs,
		   int encoding, const char *prefix)
{
  char *key, *v;
  int key_len;
  char *key_buf;
  int prefix_len = strlen(prefix);
  /**/
  if (!anthy_private_tt_dic) {
    return ;
  }
  key = anthy_xstr_to_cstr(xs, encoding);
  key_len = strlen(key);
  key_buf = malloc(key_len + 12);
  /* 辞書中には各単語が「見出し XXXX」(XXXXはランダムな文字列)を
   * キーとして保存されているので列挙する
   */
  sprintf(key_buf, "%s%s ", prefix, key);
  do {
    if (strncmp(&key_buf[2], key, key_len) ||
	strncmp(&key_buf[0], prefix, prefix_len) ||
	key_buf[key_len+2] != ' ') {
      /* 「見出し 」で始まっていないので対象外 */
      break;
    }
    /* 単語を読み出して登録 */
    v = anthy_trie_find(anthy_private_tt_dic, key_buf);
    if (v) {
      add_to_seq_ent(v, encoding, seq);
    }
    free(v);
    /**/
  } while (anthy_trie_find_next_key(anthy_private_tt_dic,
				    key_buf, key_len + 8));
  free(key);
  free(key_buf);
}

void
anthy_copy_words_from_private_dic(struct seq_ent *seq,
				  xstr *xs, int is_reverse)
{
  if (is_reverse || !anthy_private_tt_dic) {
    return ;
  }
  /* 個人辞書から取ってくる */
  copy_words_from_tt(seq, xs, ANTHY_EUC_JP_ENCODING, "  ");
  copy_words_from_tt(seq, xs, ANTHY_UTF8_ENCODING, " p");
  /* 未知語辞書から取ってくる */
  copy_words_from_tt(seq, xs, ANTHY_UTF8_ENCODING, " U");
}

static void
migrate_words(void)
{
  int encoding = anthy_dic_util_set_encoding(-1);
  if (anthy_select_section("PRIVATEDIC", 0) == -1) {
    return ;
  }
  anthy_select_first_row();
  do {
    int i, nr;
    xstr *idx_xs = anthy_get_index_xstr();
    nr = anthy_get_nr_values();
    for (i = 0; i < nr; i += 3) {
      xstr *word_xs = anthy_get_nth_xstr(i);
      int freq = anthy_get_nth_value(i+2);
      xstr *wt_xs = anthy_get_nth_xstr(i+1);
      /**/
      char *idx = anthy_xstr_to_cstr(idx_xs, encoding);
      char *word =anthy_xstr_to_cstr(word_xs, encoding);
      char *wt = anthy_xstr_to_cstr(wt_xs, encoding);
      anthy_priv_dic_add_entry(idx, word, wt, freq);
      free(idx);
      free(wt);
      free(word);
    }
  } while (anthy_select_next_row() == 0);
  /* 2005/11/13
   * 大量に単語が登録されている場合、旧形式の単語は消した方が良いかもしれない
   * もし、将来にこのコードの意味が不明であれば、この関数(migrate_words)ごと
   * 消してください
   */
  /*anthy_release_section();*/
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
  buf++;
  /* 単語 */
  res->word = buf;
  return 0;
}

static void
update_unknown_word(const char *yomi_buf, struct word_line *wl)
{
  char *word_buf = malloc(strlen(wl->word) + 20);
  sprintf(word_buf, "#T*%d %s", wl->freq, wl->word);
  anthy_trie_add(anthy_private_tt_dic, yomi_buf, word_buf);
  free(word_buf);
}

static void
do_add_unknown_word(char *yomi_cs, xstr *word, int freq)
{
  struct word_line wl;
  char *word_cs;
  wl.freq = freq;
  sprintf(wl.wt, "#T");
  word_cs = anthy_xstr_to_cstr(word, ANTHY_UTF8_ENCODING);
  wl.word = word_cs;
  update_unknown_word(yomi_cs, &wl);
  free(word_cs);
}

static void
add_unknown_word(xstr *yomi, xstr *word, int freq)
{
  char *yomi_cs, *yomi_buf;
  yomi_cs = anthy_xstr_to_cstr(yomi, ANTHY_UTF8_ENCODING);
  yomi_buf = malloc(strlen(yomi_cs) + 10);
  sprintf(yomi_buf, " U%s 0", yomi_cs);
  /* 追加 */
  do_add_unknown_word(yomi_buf, word, freq);
  free(yomi_buf);
  free(yomi_cs);
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
  add_unknown_word(yomi, word, 10);
}

void
anthy_forget_unused_unknown_word(xstr *xs)
{
  char key_buf[128];
  char *v;

  v = anthy_xstr_to_cstr(xs, ANTHY_UTF8_ENCODING);
  sprintf(key_buf, " U%s 0", v);
  free(v);
  anthy_trie_delete(anthy_private_tt_dic, key_buf);
}

void
anthy_init_private_dic(const char *id)
{
  const char *home = anthy_conf_get_str("HOME");
  if (anthy_private_tt_dic) {
    anthy_trie_close(anthy_private_tt_dic);
  }
  if (anthy_private_text_dic) {
    anthy_textdict_close(anthy_private_text_dic);
  }
  if (lock_fn) {
    free(lock_fn);
  }
  init_lock_fn(home, id);
  anthy_private_tt_dic = open_tt_dic(home, id);
  anthy_private_text_dic = open_textdic(home, id);
  /* 旧形式の辞書に含まれている単語をコピーして、texttrieに移す */
  anthy_priv_dic_lock();
  migrate_words();
  anthy_priv_dic_unlock();
}

void
anthy_release_private_dic(void)
{
  if (anthy_private_tt_dic) {
    anthy_trie_close(anthy_private_tt_dic);
    anthy_private_tt_dic = NULL;
  }
  if (anthy_private_text_dic) {
    anthy_textdict_close(anthy_private_text_dic);
    anthy_private_text_dic = NULL;
  }
  if (lock_depth > 0) {
    /* not sane situation */
    lock_depth = 0;
    if (lock_fn) {
      unlink(lock_fn);
    }
  }
  /**/
  free(lock_fn);
  lock_fn = NULL;
}

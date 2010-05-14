/*
 * 個人辞書を扱うためのコード
 * しばらくの間、個人辞書のstorageがrecordとtexttrieになる
 * 混乱をここで扱う
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <record.h>
#include <dicutil.h>
#include <conf.h>
#include <logger.h>
#include <texttrie.h>
#include <dic_ent.h>
#include <word_dic.h>
#include "dic_main.h"

/* 個人辞書 */
struct text_trie *anthy_private_tt_dic;
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

/* '\\'をエンコードしてseq_entに追加する */
static void
add_to_seq_ent(const char *v, struct seq_ent *seq, xstr *xs)
{
  int len = strlen(v);
  char *buf = malloc(len * 2);
  int i, pos = 0;
  for (i = 0; i < len; i++) {
    char c = v[i];
    if (c == ' ' || c == '\\') {
      buf[pos] = '\\';
      pos ++;
    }
    buf[pos] = c;
    pos ++;
  }
  buf[pos] = 0;
  anthy_fill_dic_ent(buf, 0, seq, xs, 0);
  free(buf);
}

/* texttrieに登録されているかをチェックし、
 * 登録されていればseq_entに追加する
 */
static void
copy_words_from_tt(struct seq_ent *seq,
		   xstr *xs)
{
  char *key, *v;
  int key_len;
  char *key_buf;
  if (!anthy_private_tt_dic) {
    return ;
  }
  key = anthy_xstr_to_cstr(xs, 0);
  key_len = strlen(key);
  key_buf = malloc(key_len + 12);
  /* 辞書中には各単語が「見出し XXXX」(XXXXはランダムな文字列)を
   * キーとして保存されているので列挙する
   */
  sprintf(key_buf, "  %s ", key);
  do {
    if (strncmp(&key_buf[2], key, key_len) ||
	key_buf[key_len+2] != ' ') {
      break;
    }
    v = anthy_trie_find(anthy_private_tt_dic, key_buf);
    if (v) {
      add_to_seq_ent(v, seq, xs);
    }
    free(v);
  } while (anthy_trie_find_next_key(anthy_private_tt_dic,
				    key_buf, key_len + 8));
  free(key);
  free(key_buf);
}

void
anthy_copy_words_from_private_dic(struct seq_ent *seq,
				  xstr *xs, int is_reverse)
{
  if (!is_reverse || !anthy_private_tt_dic) {
    copy_words_from_tt(seq, xs);
  }
}

static void
migrate_words(void)
{
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
      char *idx = anthy_xstr_to_cstr(idx_xs, 0);
      char *word =anthy_xstr_to_cstr(word_xs, 0);
      char *wt = anthy_xstr_to_cstr(wt_xs, 0);
      anthy_priv_dic_add_entry(idx, word, wt, freq);
      free(idx);
      free(wt);
      free(word);
    }
  } while (anthy_select_next_row() == 0);
  /* 2005/11/13
   * 大量に単語が登録されている場合、旧形式の単語は消した方が良いかもしれない
   * もし、将来にこのコードの意味が不明であれば、この関数ごと消してください
   */
  /*anthy_release_section();*/
}

void
anthy_init_private_dic(const char *id)
{
  const char *home = anthy_conf_get_str("HOME");
  if (anthy_private_tt_dic) {
    anthy_trie_close(anthy_private_tt_dic);
  }
  if (lock_fn) {
    free(lock_fn);
  }
  init_lock_fn(home, id);
  anthy_private_tt_dic = open_tt_dic(home, id);
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

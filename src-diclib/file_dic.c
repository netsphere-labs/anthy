/*
 * File Dictioanry
 * ファイルの辞書のインターフェース、存在するデータは
 * キャッシュされるのでここでは存在しない単語の
 * サーチを高速にする必要がある。
 *
 * anthy_file_dic_fill_seq_ent_by_xstr()が中心となる関数である
 *  指定したfile_dicから指定した文字列をインデックスとしてもつエントリに
 *  語尾を付加してseq_entに追加する
 *
 * a)辞書の形式とb)辞書アクセスの高速化c)辞書ファイルのエンコーディング
 *  このソース中で扱ってるのでかなり複雑化してます．
 *
 * Copyright (C) 2000-2003 TABATA Yusuke
 * Copyright (C) 2001-2002 TAKAI Kosuke
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include <alloc.h>
#include <dic.h>
#include <xchar.h>
#include <file_dic.h>
#include <logger.h>
#include "dic_main.h"
#include "dic_ent.h"

struct file_dic_page {
  /** そのページ中の最初の単語 */
  xstr str;
};

static allocator file_dic_ator;

int
anthy_dic_ntohl(int a)
{
  return ntohl(a);
}

static void
file_dic_dtor(void *p)
{
  struct file_dic *fdic = p;
  int i;
  if (!fdic->dic_file) {
    return ;
  }
  munmap(fdic->dic_file, fdic->file_size);
  for (i = 0; i <  fdic->nr_pages; i++) {
    free(fdic->pages[i].str.str);
  }
  if (fdic->pages) {
    free(fdic->pages);
  }
}

/* 1バイト目を見て、文字が何バイトあるかを返す */
static int
mb_fragment_len(char *str)
{
#ifdef USE_UCS4
  unsigned char *tmp = (unsigned char *)str;
  if (*tmp < 0x80) {
    return 1;
  } else if (*tmp < 0xe0) {
    return 2;
  } else if (*tmp < 0xf0) {
    return 3;
  } else if (*tmp < 0xf8) {
    return 4;
  } else if (*tmp < 0xfc) {
    return 5;
  } else {
    return 6;
  }
#else
  if ((*str) & 0x80) {
    return 2;
  }
#endif
  return 1;
}

static int
is_printable(char *str)
{
  unsigned char *tmp = (unsigned char *)str;
  if (*tmp > 31 && *tmp < 127) {
    return 1;
  }
  if (mb_fragment_len(str) > 1) {
    return 1;
  }
  return 0;
}

/* 辞書のエンコーディングからxcharを作る */
static xchar
form_mb_char(char *str)
{
#ifdef USE_UCS4
  xchar res;
  anthy_utf8_to_ucs4_xchar(str, &res);
  return res;
#else
  unsigned char *tmp = (unsigned char *)str;
  return tmp[0] * 256 + tmp[1];
#endif
}

static int
hash(xstr *x)
{
  return anthy_xstr_hash(x)&
    (YOMI_HASH_ARRAY_SIZE*YOMI_HASH_ARRAY_BITS-1);
}

static int
check_hash_ent(struct file_dic *fdic, xstr *xs)
{
  int val = hash(xs);
  int idx = (val>>YOMI_HASH_ARRAY_SHIFT)&(YOMI_HASH_ARRAY_SIZE-1);
  int bit = val & ((1<<YOMI_HASH_ARRAY_SHIFT)-1);
  return fdic->hash_ent[idx] & (1<<bit);
}

static int
wtype_str_len(char *str)
{
  int i;
  for (i = 0; str[i] && str[i]!= ' '; i++);
  return i;
}

/* 辞書の行中をスキャンするための状態保持 */
struct wt_stat {
  wtype_t wt;
  const char *wt_name;
  int freq;
  int order_bonus;/* 辞書中の順序による頻度のボーナス */
  int offset;/* 文字列中のオフセット */
  char *line;
};
/*
 * #XX*123 というCannadicの形式をパーズする
 */
static const char *
parse_wtype_str(struct wt_stat *ws)
{
  int len;
  char *buf;
  char *freq_part;
  const char *wt_name;
  /* バッファへコピーする */
  len = wtype_str_len(&ws->line[ws->offset]);
  buf = alloca(len + 1);
  strncpy(buf, &ws->line[ws->offset], len);
  buf[len] = 0;

  /* parseする */
  freq_part = strchr(buf, '*');
  if (freq_part) {
    *freq_part = 0;
    freq_part ++;
    ws->freq = atoi(freq_part) * FREQ_RATIO;
  } else {
    ws->freq = FREQ_RATIO - 2;
  }

  wt_name = anthy_type_to_wtype(buf, &ws->wt);
  if (!wt_name) {
    anthy_wtype_set_pos(&ws->wt, POS_INVAL);
  }
  ws->offset += len;
  return wt_name;
}

/** seq_entにdic_entを追加する */
static int
add_dic_ent(struct seq_ent *seq, struct wt_stat *ws,
	    int id)
{
  int i, j;
  /* 辞書ファイル中のバイト数 */
  int char_count;
  char *buf;
  xstr *xs;
  wtype_t w = ws->wt;
  int freq = ws->freq + ws->order_bonus;
  char *s = &ws->line[ws->offset];

  /* 単語の文字数を計算 */
  for (i = 0, char_count = 0;
       s[i] && (s[i] != ' ') && (s[i] != '#'); i++) {
    char_count ++;
    if (s[i] == '\\') {
      char_count++;
      i++;
    }
  }
  /**/

  /* bufに単語をコピー */
  buf = alloca(char_count+1);
  buf[char_count] = 0;
  for (j = 0; j < char_count; j++){
    buf[j] = s[j];
  }

  if (!ws->wt_name) {
    return char_count;
  }
  /**/
  xs = anthy_cstr_to_xstr(buf, 0);
  anthy_mem_dic_push_back_dic_ent(seq, xs, w, ws->wt_name, freq, id);
  if (anthy_wtype_get_meisi(w)) {
    /* 連用形が名詞化するやつは名詞化したものも追加 */
    anthy_wtype_set_ct(&w, CT_MEISIKA);
    anthy_mem_dic_push_back_dic_ent(seq, xs, w, ws->wt_name, freq, id);
  }
  anthy_free_xstr(xs);
  return char_count;
}

static void
add_compound_ent(struct seq_ent *seq, struct wt_stat *ws)
{
  int len = wtype_str_len(&ws->line[ws->offset]);
  char *buf = alloca(len);
  xstr *xs;
  strncpy(buf, &ws->line[ws->offset + 1], len - 1);
  buf[len - 1] = 0;
  xs = anthy_cstr_to_xstr(buf, 0);
  anthy_mem_dic_push_back_compound_ent(seq, xs, ws->wt, ws->freq);

  ws->offset += len;
}

/** 辞書のエントリの情報を元にseq_entをうめる */
static void
fill_dic_ent(char *dic ,int idx, struct seq_ent *ent)
{
  struct wt_stat ws;

  ws.wt_name = NULL;
  ws.freq = 0;
  ws.order_bonus = 0;
  ws.offset = 0;
  ws.line = &dic[idx];

  while (ws.line[ws.offset]) {
    if (ws.line[ws.offset] == '#') {
      if (isalpha(ws.line[ws.offset + 1])) {
	/* 品詞*頻度 */
	ws.wt_name = parse_wtype_str(&ws);
	/**/
	ws.order_bonus = FREQ_RATIO - 1;
      } else {
	/* 複合語候補 */
	add_compound_ent(ent, &ws);
      }
    } else {
      /* 単語 */
      ws.offset += add_dic_ent(ent, &ws,
			       idx + ws.offset);
      if (ws.order_bonus > 0) {
	ws.order_bonus --;
      }
    }
    if (ws.line[ws.offset] == ' ') {
      ws.offset++;
    }
  }
}

/*
 * sに書かれた文字列によってxを変更する
 * 返り値は読み進めたバイト数
 */
static int
mkxstr(char *s, xstr *x)
{
  int i, len;
  /* s[0]には巻き戻しの文字数 */
  x->len -= (s[0] - 1);
  for (i = 1; is_printable(&s[i]); i ++) {
    len = mb_fragment_len(&s[i]);
    if (len > 1) {
      /* マルチバイト */
      x->str[x->len] = form_mb_char(&s[i]);
      x->len ++;
      i += (len - 1);
    } else {
      /* 1バイト文字 */
      x->str[x->len] = s[i];
      x->len ++;
    }
  } 
  return i;
}

/** ページ中の単語の場所を調べる */
static int
search_word_in_page(xstr *x, char *s)
{
  int o = 0;
  xchar *buf;
  xstr xs;
  /* このページ中にあるもっとも長い単語を格納しうる長さ */
  buf = alloca(sizeof(xchar)*strlen(s)/2);
  xs.str = buf;
  xs.len = 0;
  while (*s) {
    s += mkxstr(s, &xs);
    if (!anthy_xstrcmp(&xs, x)) {
      return o;
    }
    o ++;
  }
  return -1;
}

/* バイナリサーチをする */
static int
get_page_index_search(struct file_dic *sd, xstr *xs, int f, int t)
{
  /* anthy_xstrcmpが-1で無くなったところを探す */
  int c,p;
  c = (f+t)/2;
  p = anthy_xstrcmp(xs, &sd->pages[c].str);
  if (p == -1) {
    /* f<= <=c */
    if (f == c-1) {
      if (anthy_xstrcmp(xs, &sd->pages[c-1].str) > -1) {
	return c-1;
      }
    }
    return get_page_index_search(sd, xs, f, c+1);
  }
  if (p == 1) {
    /* c<= <t */
    return get_page_index_search(sd, xs, c, t);
  }
  return c;
}

/** xsを含む可能性のあるページの番号を得る、
 * 範囲チェックをしてバイナリサーチを行うget_page_index_searchを呼ぶ
 */
static int
get_page_index(struct file_dic *fd, xstr *xs)
{
  int page;
  /* 最初のページの読みよりも小さい */
  if (anthy_xstrcmp(xs, &fd->pages[0].str) == -1) {
    return -1;
  }
  /* 最後のページの読みよりも大きいので、最後のページに含まれる可能性がある */
  if (anthy_xstrcmp(xs, &fd->pages[fd->nr_pages-1].str) >= 0) {
    return fd->nr_pages-1;
  }
  /* バイナリサーチ */
  page = get_page_index_search(fd, xs, 0, fd->nr_pages);
  return page;
}

/*
 * ページ中の最初の単語を取り出す
 */
static void
extract_page(struct file_dic_page *p, char *s)
{
  int i, j, l = 0;
  xstr *x = &p->str;
  /* 一つ目の読みの長さを数える */
  s++; /* 一文字目は巻戻しの文字数 */
  for (i = 0; is_printable(&s[i]);) {
    i += mb_fragment_len(&s[i]);
    l ++;
  }

  /* それをコピーする */
  x->len = l;
  x->str = malloc(sizeof(xchar) * l);
  for (i = 0, j = 0; i < x->len; i++) {
    int len = mb_fragment_len(&s[j]);
    x->str[i] = form_mb_char(&s[j]);
    j += len;
  }
}

static int
get_nr_page(struct file_dic *h)
{
  int i;
  for (i = 1; anthy_dic_ntohl(h->page_index[i]); i++);
  return i;
}

/* 辞書ファイルからページのインデックスを作成する */
static int
make_dic_index(struct file_dic *fdic)
{
  int i;

  fdic->nr_pages = get_nr_page(fdic);
  fdic->pages = malloc(sizeof(struct file_dic_page)*fdic->nr_pages);
  if (!fdic->pages) {
    return -1;
  }
  for (i = 0; i < fdic->nr_pages; i++) {
    int p = anthy_dic_ntohl(fdic->page_index[i]);
    extract_page(&fdic->pages[i], &fdic->page[p]);
  }
  return 0;
}

/* 辞書をメモリ上にmapする */
static int
map_file_dic(struct file_dic *fdic, const char *fn)
{
  int fd, r;
  struct stat st;
  char *ptr;

  fdic->dic_file = NULL;

  fd = open(fn, O_RDONLY);
  if (fd == -1) {
    anthy_log(0, "Failed to open (%s).\n", fn);
    return -1;
  }
  r = fstat(fd, &st);
  if (r == -1) {
    anthy_log(0, "Failed to stat() (%s).\n", fn);
    return -1;
  }

  fdic->file_size = st.st_size;
  ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (ptr == MAP_FAILED) {
    anthy_log(0, "Failed to mmap() (%s).\n", fn);
    return -1;
  }
  fdic->dic_file = ptr;
  return 0;
}

static char *
get_section(struct file_dic *fdic, int section)
{
  int *p = (int *)fdic->dic_file;
  int offset = anthy_dic_ntohl(p[section]);
  return &fdic->dic_file[offset];
}

/** 辞書ファイルをmmapして、file_dic中の各セクションのポインタを取得する */
static int
get_file_dic_sections(struct file_dic *fdic)
{
  fdic->entry_index = (int *)get_section(fdic, 2);
  fdic->entry = (char *)get_section(fdic, 3);
  fdic->page = (char *)get_section(fdic, 4);
  fdic->page_index = (int *)get_section(fdic, 5);
  fdic->uc_section = (char *)get_section(fdic, 6);
  fdic->hash_ent = (unsigned char *)get_section(fdic, 7);

  return 0;
}

/** 指定された単語の辞書中のインデックスを調べる */
static int
search_yomi_index(struct file_dic *fdic, xstr *xs)
{
  int p, o;
  int page_number;

  p = get_page_index(fdic, xs);

  if (p == -1) {
    return -1;
  }

  page_number = anthy_dic_ntohl(fdic->page_index[p]);
  o = search_word_in_page(xs, &fdic->page[page_number]);
  if (o == -1) {
    return -1;
  }
  return o + p * WORDS_PER_PAGE;
}

/** file_dicから単語を検索する
 * 辞書キャッシュから呼ばれる
 * 指定された活用の単語をfile_dicから探し，tailを付加して
 * mem_dic_push_back_dic_entを用いて，seq_entに追加する．
 */
void
anthy_file_dic_fill_seq_ent_by_xstr(struct file_dic *fd, xstr *xs,
				    struct seq_ent *se)
{
  int yomi_index;

  if (xs->len > 31) {
    /* 32文字以上単語には未対応 */
    return;
  }
  /* hashにないなら除去 */
  if (!check_hash_ent(fd, xs)) {
    return ;
  }

  yomi_index = search_yomi_index(fd, xs);
  se->id = yomi_index;
  if (yomi_index >= 0) {
    /* 該当する読みが辞書中にあれば、それを引数にseq_entを埋める */
    int entry_index = anthy_dic_ntohl(fd->entry_index[yomi_index]);
    se->seq_type |= ST_WORD;
    fill_dic_ent(fd->entry,
		 entry_index,
		 se);
  }
}

char *
anthy_file_dic_get_hashmap_ptr(struct file_dic *fdic)
{
  return get_section(fdic, 8);
}

struct file_dic *
anthy_create_file_dic(const char *fn)
{
  struct file_dic *fdic;
  char *p;
  int h;

  fdic = anthy_smalloc(file_dic_ator);
  memset(fdic, 0, sizeof(*fdic));

  /* 辞書ファイルをマップする
     単語辞書のhashを構成する */
  if (map_file_dic(fdic, fn) == -1 ||
      get_file_dic_sections(fdic) == -1 ||
      make_dic_index(fdic) == -1) {
    anthy_sfree(file_dic_ator, fdic);
    return 0;
  }

  /* 用例辞書をマップする */
  p = fdic->uc_section;
  if (p) {
    h = anthy_dic_ntohl(*((int *)&p[8]));
    fdic->ucs = (int *)(&fdic->uc_section[h]);
    fdic->nr_ucs = anthy_dic_ntohl(*((int*)&fdic->uc_section[12]));
  } else {
    fdic->nr_ucs = 0;
  }
  return fdic;
}

void
anthy_release_file_dic(struct file_dic *fdic)
{
  anthy_sfree(file_dic_ator, fdic);
}

void
anthy_init_file_dic(void)
{
  file_dic_ator = anthy_create_allocator(sizeof(struct file_dic),
					 file_dic_dtor);
}

/*
 * cannadic形式のファイルから辞書ファイルを作る
 *
 * Funded by IPA未踏ソフトウェア創造事業 2002 1/1
 *
 * Copyright (C) 2000-2004 TABATA Yusuke
 * Copyright (C) 2001-2002 TAKAI Kousuke
 */
/*
 * 辞書は読みをindexとし、品詞や変換後の単語(=entry)を検索
 * する構造になっている。
 *
 * 読み -> 単語、単語、、
 *
 * 辞書ファイルはネットワークバイトオーダーを用いる。
 *
 * 辞書ファイルは複数のセクションから構成されている
 *  0 ヘッダ 16*4 bytes
 *  2 読みのインデックス (読み64個ごと)
 *  3 読み
 *  4 ページ
 *  5 ページのインデックス
 *  6 用例辞書(?)
 *  7 読み hash
 *  8 用例 hash (not yet)
 *
 * source 元の辞書ファイル
 * file_dic 生成するファイル
 *
 * yomi_hash 辞書ファイルに出力されるhashのbitmap
 * index_hash このソース中でstruct yomi_entryを検索するためのhash
 *
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <config.h>

#ifdef USE_UCS4
#include <iconv.h>
#endif

#include <file_dic.h>
#include <xstr.h>
#include <wtype.h>

#include "mkdic.h"

#define MAX_LINE_LEN 1024
#define NR_HEADER_SECTIONS 16
#define SECTION_ALIGNMENT 8
#define MAX_WTYPE_LEN 20

#define DEFAULT_FN "anthy.dic"
static const char *output_fn = DEFAULT_FN;

static const char *progname;
static const char *ucfile;
static FILE *yomi_entry_index_out, *yomi_entry_out;
static FILE *page_out, *page_index_out;
static FILE *uc_out;
static FILE *yomi_hash_out;
static FILE *versatile_hash_out;
static int yomi_hash_collision;

/* ファイル中の順序に従って並べる */
struct file_section {
  FILE **fpp;
} file_array[] = {
  {&yomi_entry_index_out},
  {&yomi_entry_out},
  {&page_out},
  {&page_index_out},
  {&uc_out},
  {&yomi_hash_out},
  {&versatile_hash_out},
  {NULL},
};

#ifdef USE_UCS4
static iconv_t euc_to_utf8;
#endif


/* 辞書の出力先のファイルをオープンする */
static void
open_output_files(void)
{
  struct file_section *fs;
  for (fs = file_array; fs->fpp; fs ++) {
    if (!(*(fs->fpp) = tmpfile())) {
      fprintf (stderr, "%s: cannot open temporary file: %s\n",
	       progname, strerror (errno));
      exit (2);
    }
  }
}

static void
flush_output_files (void)
{
  struct file_section *fs;
  for (fs = file_array; fs->fpp; fs ++) {
    if (ferror(*(fs->fpp))) {
      fprintf (stderr, "%s: write error\n", progname);
      exit (1);
    }
  }
  for (fs = file_array; fs->fpp; fs ++) {
    if (fflush(*(fs->fpp))) {
      fprintf (stderr, "%s: write error: %s\n", progname, strerror (errno));
      exit (1);
    }
  }
}

/* 2つの文字列の共通部分の長さを求める */
static int
common_len(xstr *s1, xstr *s2)
{
  int m,i;
  if (!s1 || !s2) {
    return 0;
  }
  if (s1->len < s2->len) {
    m = s1->len;
  }else{
    m = s2->len;
  }
  for (i = 0; i < m; i++) {
    if (s1->str[i] != s2->str[i]) {
      return i;
    }
  }
  return m;
}

/* オリジナルの辞書ファイルのエンコーディングから
   内部のエンコーディングに変換する */
static char *
source_str_to_file_dic_str(char *str)
{
#ifdef USE_UCS4
  /* EUCからUTF8に変換する */
  char *inbuf = str;
  size_t outbytesleft = strlen(str) * 6 + 2;
  char *outbuf = alloca(outbytesleft);
  char *buf = outbuf;
  size_t inbytesleft = strlen(str);
  size_t res;
  res = iconv(euc_to_utf8, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
  if (res == (size_t) -1) {
    fprintf(stderr, "failed to iconv()\n");
    exit(1);
  }
  *outbuf = 0;
  return strdup(buf);
#else
  return strdup(str);
#endif
}

/* ネットワークbyteorderで4bytes書き出す */
void
write_nl(FILE *fp, int i)
{
  i = htonl(i);
  fwrite(&i, sizeof(int), 1, fp);
}

/*
 * 2つの文字列の差分を出力する
 * AAA ABBB という2つの文字列を見た場合には
 * ABBBはAAAのうしろ2文字を消してBBBを付けたものとして
 * \0x2BBBと出力される。
 */
static int
output_diff(xstr *p, xstr *c)
{
  int i, m, len = 1;
  m = common_len(p, c);
  if (p && p->len > m) {
    fprintf(page_out, "%c", p->len - m + 1);
  } else {
    fprintf(page_out, "%c", 1);
  }
  for (i = m; i < c-> len; i++) {
    char buf[8];
    len += anthy_sputxchar(buf, c->str[i], 0);
    fputs(buf, page_out);
  }
  return len;
}

static void
print_usage(void)
{
  printf("please use mkanthydic command.\n");
  exit(0);
}

static char *
read_line(FILE *fp, char *buf)
{
  /* 長すぎる行を無視する */
  int toolong = 0;

  while (fgets(buf, MAX_LINE_LEN, fp)) {
    int len = strlen(buf);
    if (buf[0] == '#') {
      continue ;
    }
    if (buf[len - 1] != '\n') {
      toolong = 1;
      continue ;
    }

    buf[len - 1] = 0;
    if (toolong) {
      toolong = 0;
    } else {
      return buf;
    }
  }
  return NULL;
}

/** cannadic形式の辞書の行からindexとなる部分を取り出す */
static char *
get_index_from_line(char *buf)
{
  char *sp, *res;
  sp = strchr(buf, ' ');
  if (!sp) {
    /* 辞書のフォーマットがおかしい */
    return NULL;
  }
  *sp = 0;
  res = source_str_to_file_dic_str(buf);
  *sp = ' ';
  return res;
}

/** cannadic形式の辞書の行からindex以外の部分を取り出す */
static char *
get_entry_from_line(char *buf)
{
  char *sp;
  sp = strchr(buf, ' ');
  while(*sp == ' ') {
    sp ++;
  }
  return source_str_to_file_dic_str(sp);
}

static void
begin_new_page(int i)
{
  fputc(0, page_out);
  write_nl(page_index_out, i);
}

static void
output_entry_index(int i)
{
  write_nl(yomi_entry_index_out, i);
}

static int
index_hash(xstr *xs)
{
  int i;
  unsigned int h = 0;
  for (i = 0; i < xs->len; i++) {
    h += xs->str[i];
  }
  return (int)(h % YOMI_HASH);
}

/** 配列の最後に、読みを一つを追加する */
static void
push_back_word_entry(struct yomi_entry *ye, const char *wt_name,
		     int freq, const char *word)
{
  wtype_t wt;
  if (freq == 0) {
    return ;
  }
  if (!anthy_type_to_wtype(wt_name, &wt)) {
    /* anthyの知らない品詞 */
    return ;
  }
  ye->entries = realloc(ye->entries,
			sizeof(struct word_entry) *
			(ye->nr_entries + 1));

  ye->entries[ye->nr_entries].wt = strdup(wt_name);
  ye->entries[ye->nr_entries].freq = freq;
  ye->entries[ye->nr_entries].word = strdup(word);
  ye->nr_entries ++;
}

static int
parse_wtype(char *wtbuf, char *cur)
{
  /* 品詞 */
  char *t;
  int freq;
  if (strlen(cur) >= MAX_WTYPE_LEN) {
    return 0;
  }
  strcpy(wtbuf, cur);
  t = strchr(wtbuf, '*');
  freq = 1;
  if (t) {
    int tmp_freq;
    *t = 0;
    t++;
    tmp_freq = atoi(t);
    if (tmp_freq) {
      freq = tmp_freq;
    }
  }
  return freq;
}

/** 複合候補の形式チェック */
static int
check_compound_candidate(xstr *index, const char *cur)
{
  xstr *xs = anthy_cstr_to_xstr(cur, 0);
  int i, total = 0;
  for (i = 0; i < xs->len - 1; i++) {
    if (xs->str[i] == '_') {
      total += xs->str[i+1] - '0';
    }
  }
  anthy_free_xstr(xs);
  if (total != index->len) {
    fprintf(stderr, "Invalid compound candidate (%s, length = %d).\n",
	    cur, total);
    return 0;
  }
  return 1;
}

/** 読みに対応する行を分割して、配列を構成する */
static void
push_back_word_entry_line(struct yomi_entry *ye, const char *ent)
{
  char *buf = alloca(strlen(ent) + 1);
  char *cur = buf;
  char *n;
  char wtbuf[MAX_WTYPE_LEN];
  int freq = 0;

  strcpy(buf, ent);
  wtbuf[0] = 0;

  while (1) {
    /* トークンを切る。curの後の空白か\0を探す */
    for (n = cur; *n != ' ' && *n; n++) {
      if (*n == '\\') {
	n++;
      }
    }
    if (*n) {
      *n = 0;
    } else {
      n = NULL;
    }
    if (cur[0] == '#') {
      if (isalpha(cur[1])) {
	/* #XX*?? をパース */
	freq = parse_wtype(wtbuf, cur);
      } else {
	if (cur[1] == '_' &&
	    check_compound_candidate(ye->index_xstr, &cur[1])) {
	  /* #_ 複合候補 */
	  push_back_word_entry(ye, wtbuf, freq, cur);
	}
      }
    } else {
      /* 読みを追加 */
      push_back_word_entry(ye, wtbuf, freq, cur);
    }
    if (!n) {
      return ;
    }
    cur = n;
    cur ++;
  }
}

/** 同じ単語が無いかチェック */
static int
check_same_word(struct yomi_entry *ye, int idx)
{
  struct word_entry *base = &ye->entries[idx];
  int i;
  for (i = idx -1; i >= 0; i--) {
    struct word_entry *cur = &ye->entries[i];
    if (base->freq != cur->freq) {
      return 0;
    }
    if (strcmp(base->wt, cur->wt)) {
      return 0;
    }
    if (strcmp(base->word, cur->word)) {
      return 0;
    }
    return 1;
  }
  return 0;
}

/** qsort用の比較関数 */
static int
compare_word_entry(const void *p1, const void *p2)
{
  const struct word_entry *e1 = p1;
  const struct word_entry *e2 = p2;
  return e2->freq - e1->freq;
}

/** いらない単語を消す */
static void
normalize_entry(struct yomi_entry *ye)
{
  int i;
  if (!ye) {
    return ;
  }
  /* 単語を並べる */
  qsort(ye->entries, ye->nr_entries,
	sizeof(struct word_entry),
	compare_word_entry);
  /* ダブったら、0点 */
  for (i = 0; i < ye->nr_entries; i++) {
    if (check_same_word(ye, i)) {
      ye->entries[i].freq = 0;
    }
  }
  /* 再びソート */
  qsort(ye->entries, ye->nr_entries,
	sizeof(struct word_entry),
	compare_word_entry);
}

/** 一つの読みに対する単語の内容を出力する */
static int
output_yomi_entry(struct yomi_entry *ye)
{
  int i;
  int count = 0;

  if (!ye) {
    return 0;
  }
  /* 各単語を出力する */
  for (i = 0; i < ye->nr_entries; i++) {
    struct word_entry *we = &ye->entries[i];
    /**/
    if (!we->freq) {
      continue;
    }
    if (i) {
      /* 二つ目以降は空白から始まる */
      count += fprintf(yomi_entry_out, " ");
    }
    /* 品詞と頻度を出力する */
    if (i == 0 ||
	(strcmp(ye->entries[i-1].word, we->word) ||
	 strcmp(ye->entries[i-1].wt, we->wt) ||
	 ye->entries[i-1].freq != we->freq)) {
      count += fprintf(yomi_entry_out, "%s", we->wt);
      if (we->freq > 1) {
	count += fprintf(yomi_entry_out, "*%d", we->freq);
      }
      count += fprintf(yomi_entry_out, " ");
    }
    /* 単語を出力する場所がこの単語のid */
    we->offset = count + ye->offset;
    /* 単語を出力する */
    count += fprintf(yomi_entry_out, "%s", we->word);
  }

  fputc(0, yomi_entry_out);
  return count + 1;
}

/*その読みに対応するyomi_entryを返す
**/
static struct yomi_entry *
find_yomi_entry(struct yomi_entry_list *yl, xstr *index)
{
  struct yomi_entry *ye;
  int hash = index_hash(index);
  int search = 0;
  /* hash chainから探す */
  for (ye = yl->hash[hash];ye ; ye = ye->hash_next) {
    search ++;
    if (!anthy_xstrcmp(ye->index_xstr, index)) {
      return ye;
    }
  }

  /* 無いので確保 */
  ye = malloc(sizeof(struct yomi_entry));
  ye->nr_entries = 0;
  ye->entries = 0;
  ye->next = NULL;
  ye->index_xstr = anthy_xstr_dup(index);

  /* hash chainにつなぐ */
  ye->hash_next = yl->hash[hash];
  yl->hash[hash] = ye;

  /* リストにつなぐ */

  ye->next = yl->head;
  yl->head = ye;

  yl->nr_entries ++;

  return ye;
}

/* 辞書ファイル中のhash bitmapにマークを付ける */
static void
mark_hash_array(unsigned char *hash_array, xstr *xs)
{
  int val, idx, bit, mask;
  val = anthy_xstr_hash(xs);
  val &= (YOMI_HASH_ARRAY_SIZE*YOMI_HASH_ARRAY_BITS-1);
  idx=(val>>YOMI_HASH_ARRAY_SHIFT)&(YOMI_HASH_ARRAY_SIZE-1);
  bit= val & ((1<<YOMI_HASH_ARRAY_SHIFT)-1);
  mask = (1<<bit);
  if (hash_array[idx] & mask) {
    yomi_hash_collision ++;
  }
  hash_array[idx] |= mask;
}

/* 読みhashのビットマップを作る */
static void
mk_yomi_hash(FILE *yomi_hash_out, struct yomi_entry_list *yl)
{
  unsigned char *hash_array;
  int i;
  struct yomi_entry *ye;
  hash_array = (unsigned char *)malloc(YOMI_HASH_ARRAY_SIZE);
  for (i = 0; i < YOMI_HASH_ARRAY_SIZE; i++) {
    hash_array[i] = 0;
  }
  for (i = 0; i < yl->nr_entries; i++) {
    ye = yl->ye_array[i];
    mark_hash_array(hash_array, ye->index_xstr);
  }
  fwrite(hash_array, YOMI_HASH_ARRAY_SIZE, 1, yomi_hash_out);
  printf("generated yomi hash bitmap (%d collisions/%d entries)\n",
	 yomi_hash_collision, yl->nr_entries);
	 
}

/** 辞書を一行ずつ読み込んでリストを作る
 * このコマンドのコア */
static void
parse_word_dict(FILE *fin, struct yomi_entry_list *yl)
{
  xstr *cur;
  char buf[MAX_LINE_LEN];
  char *ent, *index;
  struct yomi_entry *ye = NULL;

  while(read_line(fin, buf)) {
    index = get_index_from_line(buf);
    if (!index) {
      break;
    }
    ent = get_entry_from_line(buf);
    cur = anthy_file_dic_str_to_xstr(index);

    /* 読みが30文字を越える場合は無視 */
    if (cur->len < 31) {
      ye = find_yomi_entry(yl, cur);
      push_back_word_entry_line(ye, ent);
    }

    free(ent);
    free(index);
    anthy_free_xstr(cur);
  }
}

/* qsort用の比較関数 */
static int
compare_yomi_entry(const void *p1, const void *p2)
{
  const struct yomi_entry *const *y1 = p1;
  const struct yomi_entry *const *y2 = p2;
  return anthy_xstrcmp((*y1)->index_xstr, (*y2)->index_xstr);
}

/* yomi_entryでsortする */
static void
sort_word_dict(struct yomi_entry_list *yl)
{
  int i;
  struct yomi_entry *ye;
  yl->ye_array = malloc(sizeof(struct yomi_entry *) * yl->nr_entries);
  for (i = 0, ye = yl->head; i < yl->nr_entries; i++, ye = ye->next) {
    yl->ye_array[i] = ye;
  }
  qsort(yl->ye_array, yl->nr_entries,
	sizeof(struct yomi_entry *),
	compare_yomi_entry);
}

/** 単語辞書を出力する
 * また、このときに辞書中のオフセットも計算する */
static void
output_word_dict(struct yomi_entry_list *yl)
{
  xstr *prev = NULL;
  int entry_index = 0;
  int page_index = 0;
  struct yomi_entry *ye = NULL;
  int i;

  /* まず、最初の読みに対するエントリのインデックスを書き出す */
  write_nl(page_index_out, page_index);

  /* 各読みに対するループ */
  for (i = 0; i < yl->nr_entries; i++) {
    ye = yl->ye_array[i];
    /* 新しいページの開始 */
    if ((i % WORDS_PER_PAGE) == 0 && i) {
      page_index ++;
      prev = NULL;
      begin_new_page(page_index);
    }

    page_index += output_diff(prev, ye->index_xstr);
    output_entry_index(entry_index);
    ye->offset = entry_index;
    normalize_entry(ye);
    entry_index += output_yomi_entry(ye);
    /***/
    prev = ye->index_xstr;
  }

  /* 最後の読みを終了 */
  entry_index += output_yomi_entry(ye);
  write_nl(yomi_entry_index_out, entry_index);
  write_nl(page_index_out, 0);
  printf("Total %d words (%d pages).\n",
	 yl->nr_entries,
	 yl->nr_entries / WORDS_PER_PAGE + 1);
}

/** ファイルのサイズを取得する */
static int
get_size(FILE *fp)
{
  if (!fp) {
    return 0;
  }
  return (ftell (fp) + SECTION_ALIGNMENT - 1) & (-SECTION_ALIGNMENT);
}

static void
copy_file(FILE *in, FILE *out)
{
  int i;
  size_t nread;
  char buf[BUFSIZ];

  /* Pad OUT to the next aligned offset.  */
  for (i = ftell (out); i & (SECTION_ALIGNMENT - 1); i++) {
    fputc (0, out);
  }

  /* Copy the contents.  */
  rewind (in);
  while ((nread = fread (buf, 1, sizeof buf, in)) > 0) {
    if (fwrite (buf, 1, nread, out) < nread) {
      /* Handle short write (maybe disk full).  */
      fprintf (stderr, "%s: %s: write error: %s\n",
	       progname, output_fn, strerror (errno));
      exit (1);
    }
  }
}

static void
generate_header(FILE *fp)
{
  int buf[NR_HEADER_SECTIONS];
  int i;
  struct file_section *fs;
  int off;

  /* 初期化 */
  for (i = 0; i < NR_HEADER_SECTIONS; i++) {
    buf[i] = 0;
  }

  /* ヘッダ */
  buf[0] = NR_HEADER_SECTIONS * sizeof(int);
  buf[1] = 0;

  /* 各セクションのオフセット */
  off = buf[0];
  for (i = 2, fs = file_array; fs->fpp; fs ++, i++) {
    buf[i] = off;
    off += get_size(*(fs->fpp));
  }

  /* ファイルへ出力する */
  for (i = 0; i < NR_HEADER_SECTIONS; i++) {
    write_nl(fp, buf[i]);
  }
}

/* 各セクションのファイルをマージして、ひとつの辞書ファイルを作る */
static void
link_dics(void)
{
  FILE *fp;
  struct file_section *fs;

  fp = fopen (output_fn, "w");
  if (!fp) {
      fprintf (stderr, "%s: %s: cannot create: %s\n",
	       progname, output_fn, strerror (errno));
      exit (1);
  }

  /* ヘッダを出力する */
  generate_header(fp);

  /* 各セクションのファイルを結合する */
  for (fs = file_array; fs->fpp; fs ++) {
    copy_file(*(fs->fpp), fp);
  }

  if (fclose (fp)) {
    fprintf (stderr, "%s: %s: write error: %s\n",
	     progname, output_fn, strerror (errno));
    exit (1);
  }
}

static void
setup_versatile_hash(struct versatile_hash *vh)
{
  vh->buf = malloc(VERSATILE_HASH_SIZE);
  memset(vh->buf, 0, VERSATILE_HASH_SIZE);
}

static void
write_out_versatile_hash(struct versatile_hash *vh)
{
  int i;
  int nr = 0;
  for (i = 0; i < VERSATILE_HASH_SIZE; i++) {
    if (vh->buf[i]) {
      nr ++;
    }
  }
  printf("versatile hash density %d/%d\n", nr, VERSATILE_HASH_SIZE);
  fwrite(vh->buf, VERSATILE_HASH_SIZE, 1, versatile_hash_out);
  free(vh->buf);
}

int
main(int argc, char **argv)
{
  struct yomi_entry_list yl;
  struct versatile_hash vh;
  struct uc_dict *ud;
  int i, res;

  res = anthy_init_xstr();
  if (res == -1) {
    fprintf (stderr, "failed to init dic lib\n");
    exit(1);
  }

  progname = argv[0];
#ifdef USE_UCS4
  euc_to_utf8 = iconv_open("UTF-8", "EUC-JP");
  if (euc_to_utf8 == (iconv_t) -1) {
    fprintf(stderr, "failed in iconv_open(%s)", strerror(errno));
    exit(1);
  }
#endif


  /* 単語辞書を作る */
  yl.head = NULL;
  yl.nr_entries = 0;
  for (i = 0; i < YOMI_HASH; i++) {
    yl.hash[i] = NULL;
  }
  /* 引数で指定されたファイルを読み込む */
  for (i = 1; i < argc; i++) {
    FILE *fp;
    if (!strcmp(argv[i], "--help")) {
      print_usage();
    }
    if (i + 1 < argc) {
      if (!strcmp(argv[i], "-o")) {
	output_fn = argv[i + 1];
	i++;
	continue;
      } else if (!strcmp(argv[i], "-uc")) {
	ucfile = argv[i + 1];
	printf("uc = %s\n", ucfile);
	i++;
	continue;
      }
    }
    /* ファイル名が指定されたので読み込む */
    fp = fopen(argv[i], "r");
    if (fp) {
      parse_word_dict(fp, &yl);
    }
  }
  /* 並び替える */
  sort_word_dict(&yl);

  /* 単語辞書を出力する */
  open_output_files();
  output_word_dict(&yl);

  /* 読みハッシュを作る */
  mk_yomi_hash(yomi_hash_out, &yl);

  /* 用例を読み込む */
  ud = read_uc_file(ucfile, yl.head);

  /* 用例辞書を作る */
  make_ucdic(uc_out, ud);

  /* 汎用ハッシュを作る */
  setup_versatile_hash(&vh);

  /* 用例辞書をハッシュに書き込む */
  fill_uc_to_hash(&vh, ud);

  /* 汎用ハッシュを書き出す */
  write_out_versatile_hash(&vh);

  /* 辞書ファイルにまとめる */
  flush_output_files();
  link_dics();

  return 0;
}

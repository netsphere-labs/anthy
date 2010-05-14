/*
 * Anthy内部で使う文字列の処理
 *  typedef struct xstr_ {
 *    xstr *str; int len;
 *  } xstr;
 *
 * malloc(0);の意味は考えないで0文字の文字列を扱えるような
 * コーディングをする。free(0)は良い。
 *
 * デフォルトの設定では
 *  cstrはCの普通のEUC文字列
 *  UCS化の作業が進行中
 *
 * Copyright (C) 2000-2003 TABATA Yusuke
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
/* for ANTHY_*_ENCODING */
#include <anthy.h>
#ifdef USE_UCS4
#include <iconv.h>
#endif

#include <xstr.h>
#include "dic_main.h"
#include "xchar.h"

#ifdef USE_UCS4
static iconv_t euc_to_utf8;
static iconv_t utf8_to_euc;
#endif

/* 画面に出力するときのエンコーディング */
static int print_encoding;

#define MAX_BYTES_PER_XCHAR 10

static int
xc_isprint(xchar xc)
{
  return xc > 0;
}


#ifdef USE_UCS4
static char *
do_iconv(const char *str, iconv_t ic)
{
  int len = strlen(str);
  unsigned int buflen = len * 6+3;
  char *realbuf = alloca(buflen);
  char *outbuf = realbuf;
  const char *inbuf = str;
  memset(realbuf, 0, buflen);
  iconv(ic, &inbuf, &len, &outbuf, &buflen);
  return strdup(realbuf);
}
#endif

/** Cの文字列に対応するxstrの長さを計算する
 */
static int
xlengthofcstr(const char *c)
{
  int ll = 0;
  int len = strlen(c);
  int i;
  for (i = 0; i < len; i++) {
    ll ++;
    if ((c[i] & 0x80)) {
      i++;
    }
  }
  return ll;
}

#ifdef USE_UCS4
const char *
anthy_utf8_to_ucs4_xchar(const char *s, xchar *res)
{
  const unsigned char *str = (const unsigned char *)s;
  int i, len;
  xchar cur;
  cur = str[0];
  if (str[0] < 0x80) {
    len = 1;
  } else if (str[0] < 0xe0) {
    cur &= 0x1f;
    len = 2;
  } else if (str[0] < 0xf0) {
    cur &= 0x0f;
    len = 3;
  } else if (str[0] < 0xf8) {
    cur &= 0x07;
    len = 4;
  } else if (str[0] < 0xfc) {
    cur &= 0x03;
    len = 5;
  } else {
    cur &= 0x01;
    len = 6;
  }
  str ++;
  for (i = 1; i < len; i++) {
    cur <<= 6;
    cur |= (str[0] & 0x3f);
    str++;
  }
  *res = cur;
  return (const char *)str;
}

static xstr *
utf8_to_ucs4_xstr(const char *s)
{
  const unsigned char *str = (const unsigned char *)s;
  xstr res;
  res.str = (xchar *)alloca(sizeof(xchar) * strlen(s));
  res.len = 0;

  while (*str) {
    xchar cur;
    str = (const unsigned char *)anthy_utf8_to_ucs4_xchar((const char *)str,
							  &cur);
    res.str[res.len] = cur;
    res.len ++;
  }
  return anthy_xstr_dup(&res);
}

static int
put_xchar_to_utf8_str(xchar xc, char *buf_)
{
  int i, len;
  unsigned char *buf = (unsigned char *)buf_;
  if (xc < 0x80) {
    buf[0] = 0;
    len = 1;
  } else if (xc < 0x800) {
    buf[0] = 0xc0;
    len = 2;
  } else if (xc < 0x10000) {
    buf[0] = 0xe0;
    len = 3;
  } else if (xc < 0x200000) {
    buf[0] = 0xf0;
    len = 4;
  } else if (xc < 0x400000) {
    buf[0] = 0xf8;
    len = 5;
  } else {
    buf[0] = 0xfc;
    len = 6;
  }
  for (i = len - 1; i > 0; i--) {
    buf[i] = (xc & 0x3f) | 0x80;
    xc >>= 6;
  }
  buf[0] += xc;
  buf[len] = 0;
  return len;
}

static char *
ucs4_xstr_to_utf8(xstr *xs)
{
  char *buf = alloca(xs->len * 6 + 1);
  int i, t = 0;
  buf[0] = 0;
  for (i = 0; i < xs->len; i++) {
    xchar xc = xs->str[i];
    put_xchar_to_utf8_str(xc, &buf[t]);
    t = strlen(buf);
  }
  return strdup(buf);
}
#endif /*USE_UCS4*/

static xstr *
euc_cstr_to_euc_xstr(const char *c)
{
  xstr *x;
  int i, j, l;
  l = xlengthofcstr(c);
  x = (xstr *)malloc(sizeof(struct xstr_));
  x->len = l;
  if (l) {
    x->str = malloc(sizeof(xchar)*l);
  } else {
    x->str = 0;
  }
  for (i = 0, j = 0; i < l; i++) {
    if (!(c[j] & 0x80)){
      x->str[i] = c[j];
      j++;
    } else {
      unsigned char *p = (unsigned char *)&c[j];
      x->str[i] = (p[1] | (p[0]<<8)) | 0x8080;
      j++;
      j++;
    }
  }
  return x;
}
/** Cの文字列をxstrに変更する
 */
xstr *
anthy_cstr_to_xstr(const char *c, int encoding)
{
#ifdef USE_UCS4
  if (encoding == ANTHY_EUC_JP_ENCODING) {
    char *u;
    xstr *r;
    u = do_iconv(c, euc_to_utf8);
    r = utf8_to_ucs4_xstr(u);
    free(u);
    return r;
  } else {
    return utf8_to_ucs4_xstr(c);
  }
#endif
  /* EUC-JPの場合 */
  (void)encoding;
  return euc_cstr_to_euc_xstr(c);
}

char *
anthy_xstr_to_cstr(xstr *s, int encoding)
{
  int i, j, l;
  char *p;
#ifdef USE_UCS4
  if (encoding != ANTHY_EUC_JP_ENCODING) {
    return ucs4_xstr_to_utf8(s);
  } else {
    char *tmp = ucs4_xstr_to_utf8(s);
    p = do_iconv(tmp, utf8_to_euc);
    free(tmp);
    return p;
  }
#endif
  (void)encoding;
  l = s->len;
  for (i = 0; i < s->len; i++) {
    if (s->str[i] > 255) {
      l++;
    }
  }
  p = (char *)malloc(l + 1);
  p[l] = 0;
  j = 0;
  for (i =  0; i < s->len; i++) {
    if (s->str[i] < 256) {
      p[j] = s->str[i];
      j++;
    }else{
      p[j] = s->str[i] >> 8;
      j++;
      p[j] = s->str[i] & 255;
      j++;
    }
  }
  return p;
}

xstr *
anthy_file_dic_str_to_xstr(const char *str)
{
#ifdef USE_UCS4
  /* UTF8からUCS4へ */
  return utf8_to_ucs4_xstr(str);
#else
  return anthy_cstr_to_xstr(str, 0);
#endif
}

char *anthy_xstr_to_file_dic_str(xstr *xs)
{
#ifdef USE_UCS4
  return ucs4_xstr_to_utf8(xs);
#else
  return anthy_xstr_to_cstr(xs, 0);
#endif
}


xstr *
anthy_xstr_dup(xstr *s)
{
  int i;
  xstr *x = (xstr *)malloc(sizeof(xstr));
  x->len = s->len;
  if (s->len) {
    x->str = malloc(sizeof(xchar)*s->len);
  }else{
    x->str = 0;
  }
  for (i = 0; i < x->len; i++) {
    x->str[i] = s->str[i];
  }
  return x;
}

xchar *
anthy_xstr_dup_str(xstr *s)
{
  xchar *c;
  int i;
  if (s->len) {
    c = malloc(sizeof(xchar)*s->len);
  }else{
    c = 0;
  }
  for (i = 0; i < s->len; i++) {
    c[i] = s->str[i];
  }
  return c;
}

void
anthy_free_xstr(xstr *x)
{
  free(x->str);
  free(x);
}

void
anthy_free_xstr_str(xstr *x)
{
  free(x->str);
}

int
anthy_sputxchar(char *buf, xchar x, int encoding)
{
  if (!xc_isprint(x)) {
    sprintf(buf, "??");
    return 2;
  }
#ifdef USE_UCS4
  if (encoding == ANTHY_EUC_JP_ENCODING) {
    char tmp[10], *p;
    put_xchar_to_utf8_str(x, tmp);
    p = do_iconv(tmp, utf8_to_euc);
    strcpy(buf, p);
    free(p);
    return strlen(buf);
  } else {
    return put_xchar_to_utf8_str(x, buf);
  }
#else
  (void)encoding;
  if (x < 256) {
    buf[0] = x;
    buf[1] = 0;
    return 1;
  }
  buf[2] = 0;
  buf[1] = 0x80 | (x & 255);
  buf[0] = 0x80 | ((x>>8) & 255);
  return 2;
#endif
}

int
anthy_sputxstr(char *buf, xstr *x)
{
  char b[MAX_BYTES_PER_XCHAR];
  int i, l = 0;
  for (i = 0; i < x->len; i++) {
    anthy_sputxchar(b, x->str[i], 0);
    sprintf(&buf[l], b);
    l += strlen(b);
  }
  return l;
}

int
anthy_snputxstr(char *buf, int n, xstr *x, int encoding)
{
  char b[MAX_BYTES_PER_XCHAR];
  int i, l=0;
  for (i = 0; i < x->len; i++) {
    anthy_sputxchar(b, x->str[i], encoding);
    if ((int)strlen(b) + l >= n) {
      return l;
    }
    n -= sprintf(&buf[l], b);
    l += strlen(b);
  }
  return l;
}

void
anthy_putxchar(xchar x)
{
  char buf[MAX_BYTES_PER_XCHAR];
  if (!xc_isprint(x)) {
    printf("\\%x", x);
    return ;
  }
  anthy_sputxchar(buf, x, print_encoding);
  printf(buf);
}

void
anthy_putxstr(xstr *x)
{
  int i;
  for (i = 0; i < x->len; i++) {
    anthy_putxchar(x->str[i]);
  }
}

void
anthy_putxstrln(xstr *x)
{
  anthy_putxstr(x);
  printf("\n");
}

xstr*
anthy_xstrcpy(xstr *dest, xstr *src)
{
  int i;
  /* 文字列をコピー */
  dest->len = src->len;
  for (i = 0; i < src->len; i++) {
    dest->str[i] = src->str[i];
  }
  
  return dest;
}
/* 返り値の符号はstrcmpと同じ */
int
anthy_xstrcmp(xstr *x1, xstr *x2)
{
  int i, m;
  if (x1->len < x2->len) {
    m = x1->len;
  }else{
    m = x2->len;
  }
  for (i = 0 ; i < m ; i++) {
    if (x1->str[i] < x2->str[i]) {
      return -1;
    }
    if (x1->str[i] > x2->str[i]) {
      return 1;
    }
  }
  if (x1->len < x2->len) {
    return -1;
  }
  if (x1->len > x2->len) {
    return 1;
  }
  return 0;
}

xstr *
anthy_xstrcat(xstr *s, xstr *a)
{
  int i, l;
  l = s->len + a->len;
  if (l) {
    s->str = realloc(s->str, sizeof(xchar)*l);
  } else {
    s->str = 0;
  }
  for (i = 0; i < a->len; i ++) {
    s->str[s->len+i] = a->str[i];
  }
  s->len = l;
  return s;
}

xstr *
anthy_xstrappend(xstr *xs, xchar xc)
{
  xstr p;
  xchar q[1];
  p.len = 1;
  p.str = q;
  q[0] = xc;
  return anthy_xstrcat(xs, &p);
}

long long
anthy_xstrtoll(xstr *x)
{
  xchar c;
  int i;
  long long n = 0;/* 数 */
  if (!x->len || x->len > 16) {
    return -1;
  }
  if (!anthy_get_xstr_type(x) & (XCT_NUM | XCT_WIDENUM)) {
    return -1;
  }
  for (i = 0; i < x->len; i++) {
    c = x->str[i];
    n *= 10;
    n += anthy_xchar_to_num(c);
  }
  return n;
}

/** 平仮名をカタカナに変換する
 */
xstr *
anthy_xstr_hira_to_kata(xstr *src_xs)
{
  xstr *dst_xs;
  int i, j;
  dst_xs = anthy_xstr_dup(src_xs);

  for (i = 0 ,j = 0; i < dst_xs->len; i++, j++) {
    /* 「う゛」のチェック */
    if (i < dst_xs->len - 1 && dst_xs->str[i] == HK_U
	&& dst_xs->str[i+1] == HK_DDOT) {
      dst_xs->str[j] = 0xa5f4;/* ヴ */
      i++;
      continue ;
    }
    /**/
    dst_xs->str[j] = dst_xs->str[i];
    if ((dst_xs->str[j] & 0xff00) == 0xa400) {
      /* ひらがなだったら256足す */
      dst_xs->str[j] += 256;
    }
  }
  dst_xs->len = j;
  return dst_xs;
}

int anthy_xstr_hash(xstr *xs)
{
  int h,i;
  h = 0;
  for (i = 0 ;i < xs->len ;i++) {
    h *= 97;
    h += xs->str[i]<<4;
    h += xs->str[i]>>4;
  }
  return h;
}

void
anthy_xstr_set_print_encoding(int encoding)
{
  print_encoding = encoding;
}

int
anthy_init_xstr(void)
{
#ifdef USE_UCS4
  euc_to_utf8 = iconv_open("UTF-8", "EUC-JP");
  if (euc_to_utf8 == (iconv_t)-1) {
    return -1;
  }
  utf8_to_euc = iconv_open("EUC-JP", "UTF-8");
  if (utf8_to_euc == (iconv_t)-1) {
    iconv_close(euc_to_utf8);
    return -1;
  }
#endif
  return 0;
}

void anthy_quit_xstr(void)
{
#ifdef USE_UCS4
  iconv_close(euc_to_utf8);
  iconv_close(utf8_to_euc);
#endif
}

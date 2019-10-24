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
 *
 * Copyright (C) 2000-2007 TABATA Yusuke
 *
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
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef _WIN32
  #define strdup _strdup
#endif

/* for ANTHY_*_ENCODING */
#include <anthy/anthy.h>
#include <anthy/xstr.h>
#include <anthy/xchar.h>
#include "diclib_inner.h"

/* 画面に出力するときのエンコーディング */
static int print_encoding;

#define MAX_BYTES_PER_XCHAR 10

static int
xc_isprint(xchar xc)
{
  return xc > 0;
}

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

/**
 * Convert bytes of one code point.
 * @param [out] res converted Unicode code point. If invalid sequence,
 *                  (xchar) -1.
 * @return pointer advanced by one code point from s. If there was an invalid
 *         sequence, NULL.
 */
const char *
anthy_utf8_to_ucs4_xchar(const char *s, xchar *res)
{
  assert(s);
  assert(res);

  const unsigned char *str = (const unsigned char *)s;
  int i, len;
  xchar cur;
  cur = str[0];
  if (str[0] < 0x80) {
    len = 1;
  }
  else if (*str <= 0xc1) {
    *res = (xchar) -1;
    return NULL;
  }
  else if (str[0] < 0xe0) {
    cur &= 0x1f;
    len = 2;
  } else if (str[0] < 0xf0) {
    cur &= 0x0f;
    len = 3;
  } else if (str[0] <= 0xf4) {
    cur &= 0x07;
    len = 4;
  }
  else {
    *res = (xchar) -1;
    return NULL;
  }

  str ++;
  for (i = 1; i < len; i++) {
    if ((*str & 0xc0) != 0x80) { // Byte 2-4 is 10xxxxxx
      *res = (xchar) -1;
      return NULL;
    }
    cur <<= 6;
    cur |= (str[0] & 0x3f);
    str++;
  }
  if (cur >= 0xd800 && cur <= 0xdfff) { // surrogate pair
    *res = (xchar) -1;
    return NULL;
  }
  *res = cur;
  return (const char *)str;
}


/**
 * Convert UTF-8 C string to xstr.
 * @param s  NUL terminated UTF-8 C string.
 * @return new xstr. The caller has to free it with anthy_free_xstr().
 *         If the input is an invalid byte sequence, NULL.
 */
static xstr *
utf8_to_ucs4_xstr(const char *s)
{
  assert(s);

  const char* str = s;
  xstr* res = (xstr*) malloc(sizeof(xstr));
  if (!res)
    return NULL;
  res->len = 0;
  if (*s == '\0') {
    res->str = NULL;
    return res;
  }
  res->str = (xchar*) malloc(sizeof(xchar) * strlen(s));
  if (!res->str) {
    anthy_free_xstr(res);
    return NULL;
  }

  while (*str) {
    xchar cur;
    str = anthy_utf8_to_ucs4_xchar(str, &cur);
    if (!str) { // error
      anthy_free_xstr(res);
      return NULL;
    }
    res->str[res->len] = cur;
    res->len ++;
  }
  return res;
}

static int
put_xchar_to_utf8_str(xchar xc, char *buf_)
{
  assert( xc <= 0x1fffff );
  assert( !(xc >= 0xd800 && xc <= 0xdfff) );

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
  } /*else if (xc < 0x400000) {
    buf[0] = 0xf8;
    len = 5;
  } else {
    buf[0] = 0xfc;
    len = 6;
  }
    */
  for (i = len - 1; i > 0; i--) {
    buf[i] = (xc & 0x3f) | 0x80;
    xc >>= 6;
  }
  buf[0] += xc;
  buf[len] = '\0';
  return len;
}

/**
 * Create a new UTF-8 C string.
 */
static char *
ucs4_xstr_to_utf8(const xstr* xs)
{
  assert(xs);

  char* buf = malloc(xs->len * 4 + 1);
  if (!buf)
    return NULL;

  int i, t = 0;
  buf[0] = '\0';
  for (i = 0; i < xs->len; i++) {
    xchar xc = xs->str[i];
    put_xchar_to_utf8_str(xc, &buf[t]);
    t = strlen(buf);
  }
  return buf;
}

/**
 * Cの文字列 (encoding に従う) をxstrに変更する
 * @return 新しく生成された xstr. 呼出し側で anthy_free_xstr() すること。
 *         c が不正なバイト列だった場合, NULL.
 */
xstr *
anthy_cstr_to_xstr(const char *c, int encoding)
{
  xstr *x;
  int i, j, l;
  if (encoding == ANTHY_UTF8_ENCODING) {
    return utf8_to_ucs4_xstr(c);
  }
  l = xlengthofcstr(c);
  x = (xstr *)malloc(sizeof(struct xstr_));
  if (!x) {
    return NULL;
  }
  x->len = l;
  x->str = malloc(sizeof(xchar)*l);
  for (i = 0, j = 0; i < l; i++) {
    if (!(c[j] & 0x80)){
      x->str[i] = c[j];
      j++;
    } else {
      unsigned char *p = (unsigned char *)&c[j];
      x->str[i] = (p[1] | (p[0]<<8)) | 0x8080;
      x->str[i] = anthy_euc_to_ucs(x->str[i]);
      j++;
      j++;
    }
  }
  return x;
}


/**
 * Convert xstr to new C string under specific encoding.
 * @return new C string. The caller has to free() it.
 */
char *
anthy_xstr_to_cstr(const xstr* s, int encoding)
{
  int i, j, l;
  char *p;

  if (encoding == ANTHY_UTF8_ENCODING) {
    return ucs4_xstr_to_utf8(s);
  }

  l = s->len;
  for (i = 0; i < s->len; i++) {
    int ec = anthy_ucs_to_euc(s->str[i]);
    if (ec > 255) {
      l++;
    }
  }
  p = (char *)malloc(l + 1);
  if (!p)
    return NULL;
  p[l] = 0;
  j = 0;
  for (i =  0; i < s->len; i++) {
    int ec = anthy_ucs_to_euc(s->str[i]);
    if (ec < 256) {
      p[j] = ec;
      j++;
    }else{
      p[j] = ec >> 8;
      j++;
      p[j] = ec & 255;
      j++;
    }
  }
  return p;
}


/**
 * Duplicate a xstr.
 * @return If failed, NULL.
 */
xstr *
anthy_xstr_dup(const xstr* s)
{
  assert(s);

  xstr *x = (xstr *)malloc(sizeof(xstr));
  if (!x)
    return NULL;
  x->len = s->len;
  if (s->len) {
    int i;

    x->str = malloc(sizeof(xchar)*s->len);
    for (i = 0; i < x->len; i++) {
      x->str[i] = s->str[i];
    }
  }else{
    x->str = NULL;
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
  if (!x) {
    return ;
  }
  /**/
  free(x->str);
  free(x);
}

void
anthy_free_xstr_str(xstr *x)
{
  if (!x) {
    return ;
  }
  free(x->str);
  x->str = NULL;
}


/**
 * @return the number of bytes written to buf (except terminal NUL).
 */
int
anthy_sputxchar(char *buf, xchar x, int encoding)
{
  assert(buf);

  if (!xc_isprint(x)) {
    sprintf(buf, "??");
    return 2;
  }
  if (encoding == ANTHY_UTF8_ENCODING) {
    return put_xchar_to_utf8_str(x, buf);
  }
  x = anthy_ucs_to_euc(x);
  if (x < 256) {
    buf[0] = x;
    buf[1] = 0;
    return 1;
  }
  buf[2] = 0;
  buf[1] = 0x80 | (x & 255);
  buf[0] = 0x80 | ((x>>8) & 255);
  return 2;
}

int
anthy_sputxstr(char *buf, xstr *x, int encoding)
{
  char b[MAX_BYTES_PER_XCHAR];
  int i, l = 0;
  for (i = 0; i < x->len; i++) {
    anthy_sputxchar(b, x->str[i], encoding);
    sprintf(&buf[l], "%s", b);
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
    n -= sprintf(&buf[l], "%s", b);
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
  printf("%s", buf);
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


/**
 * Copy src to dest.
 * @return the pointer to dest
 */
xstr*
anthy_xstrcpy(xstr* dest, const xstr* src)
{
  assert(src);
  assert(dest);

  if (src == dest)
    return dest;

  if (!src->len) {
    dest->len = 0;
    free(dest->str);
    dest->str = NULL;
    return dest;
  }

  if (src->len > dest->len) {
    free(dest->str);
    dest->str = malloc(sizeof(xchar) * src->len);
    assert(dest->str);
  }
  /* 文字列をコピー */
  dest->len = src->len;
  memcpy(dest->str, src->str, sizeof(xchar) * src->len);

  return dest;
}

/* 返り値の符号はstrcmpと同じ */
int
anthy_xstrcmp(const xstr* x1, const xstr* x2)
{
  assert(x1);
  assert(x2);

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
    else if (x1->str[i] > x2->str[i]) {
      return 1;
    }
  }
  if (x1->len < x2->len) {
    return -1;
  }
  else if (x1->len > x2->len) {
    return 1;
  }
  return 0;
}

/* 返り値の符号はstrncmpと同じ */
int
anthy_xstrncmp(const xstr* x1, const xstr* x2, int n)
{
  assert(x1);
  assert(x2);

  int i, m;
  if (x1->len < x2->len) {
    m = x1->len;
  }else{
    m = x2->len;
  }
  if (m > n) m = n;
  for (i = 0 ; i < m ; i++) {
    if (x1->str[i] < x2->str[i]) {
      return -1;
    }
    if (x1->str[i] > x2->str[i]) {
      return 1;
    }
  }
  if (x2->len <= n && x1->len < x2->len) {
    return -1;
  }
  if (x1->len <= n && x1->len > x2->len) {
    return 1;
  }
  return 0;
}


xstr *
anthy_xstrcat(xstr *s, const xstr* a)
{
  assert(s);
  assert(a);

  int i, l;
/*if (!s) {
    s = malloc(sizeof(xstr));
    s->str = NULL;
    s->len = 0;
  } */
  l = s->len + a->len;

  if (l < 1) {              /* 辞書もしくは学習データが壊れていた時の対策 */
    free(s->str);
    s->str = NULL;
    s->len = 0;
    return s;
  }
  if (a->len > 0) {
    s->str = realloc(s->str, sizeof(xchar) * l);
    assert(s->str);
    memcpy(s->str + s->len, a->str, sizeof(xchar) * a->len);
    s->len = l;
  }
  return s;
}


/**
 * Append a xchar.
 */
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
  if (!(anthy_get_xstr_type(x) & (XCT_NUM | XCT_WIDENUM))) {
    return -1;
  }
  for (i = 0; i < x->len; i++) {
    c = x->str[i];
    n *= 10;
    n += anthy_xchar_to_num(c);
  }
  return n;
}

/** 全角の数字を半角にする
 */
xstr *
anthy_xstr_wide_num_to_num(xstr* src_xs)
{
  int i;
  xstr *dst_xs;
  dst_xs = anthy_xstr_dup(src_xs);
  for (i = 0; i < src_xs->len; ++i) {
    dst_xs->str[i] = anthy_xchar_wide_num_to_num(src_xs->str[i]);
  }
  return dst_xs;
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
      dst_xs->str[j] = KK_VU;/* ヴ */
      i++;
      continue ;
    }
    /**/
    dst_xs->str[j] = dst_xs->str[i];
    if ((anthy_ucs_to_euc(dst_xs->str[j]) & 0xff00) == 0xa400) {
      /* ひらがなだったら256足す */
      dst_xs->str[j] = anthy_ucs_to_euc(dst_xs->str[j]);
      dst_xs->str[j] += 256;
      dst_xs->str[j] = anthy_euc_to_ucs(dst_xs->str[j]);
    }
  }
  dst_xs->len = j;
  return dst_xs;
}

xstr *
anthy_xstr_hira_to_half_kata(xstr *src_xs)
{
  int len = src_xs->len;
  int i, j;
  xstr *xs;
  for (i = 0; i < src_xs->len; i++) {
    const struct half_kana_table *tab = anthy_find_half_kana(src_xs->str[i]);
    if (tab && tab->mod) {
      len ++;
    }
  }
  xs = malloc(sizeof(xstr));
  xs->len = len;
  xs->str = malloc(sizeof(xchar) * len);
  j = 0;
  for (i = 0; i < src_xs->len; i++) {
    const struct half_kana_table *tab = anthy_find_half_kana(src_xs->str[i]);
    if (tab) {
      xs->str[j] = anthy_euc_to_ucs(tab->dst);
      if (tab->mod) {
	j++;
	xs->str[j] = anthy_euc_to_ucs(tab->mod);
      }
    } else {
      xs->str[j] = src_xs->str[i];
    }
    j++;
  }
  return xs;
}

xstr *
anthy_conv_half_wide(xstr *xs)
{
  int i;
  xstr *res;
  for (i = 0; i < xs->len; i++) {
    if (!anthy_lookup_half_wide(xs->str[i])) {
      return NULL;
    }
  }
  res = anthy_xstr_dup(xs);
  for (i = 0; i < xs->len; i++) {
    res->str[i] = anthy_lookup_half_wide(xs->str[i]);
  }
  return res;
}

int
anthy_xstr_hash(xstr *xs)
{
  int h,i;
  h = 0;
  for (i = 0 ;i < xs->len ;i++) {
    h *= 97;
    h += xs->str[i]<<4;
    h += xs->str[i]>>4;
  }
  if (h < 0) {
    return -h;
  }
  return h;
}


/**
 * @return If failed, NULL.
 */
static char *
conv_cstr(const char *s, int from, int to)
{
  char *res;
  xstr *xs = anthy_cstr_to_xstr(s, from);
  if (!xs) {
    return NULL;
  }
  res = anthy_xstr_to_cstr(xs, to);
  anthy_free_xstr(xs);
  return res;
}

char *
anthy_conv_euc_to_utf8(const char *s)
{
  return conv_cstr(s, ANTHY_EUC_JP_ENCODING, ANTHY_UTF8_ENCODING);
}

char *
anthy_conv_utf8_to_euc(const char *s)
{
  return conv_cstr(s, ANTHY_UTF8_ENCODING, ANTHY_EUC_JP_ENCODING);
}

void
anthy_xstr_set_print_encoding(int encoding)
{
  print_encoding = encoding;
}

int
anthy_init_xstr(void)
{
  return 0;
}

void anthy_quit_xstr(void)
{
}

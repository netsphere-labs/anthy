/*
 * 文字(xchar)のタイプなどを扱う
 *
 * Copyright (C) 2001-2004 TABATA Yusuke
 */
#include "config.h"
#include <xstr.h>

#include "dic_main.h"
#include "xchar.h"

static struct xchar_ent{
  xchar xc;
  int type;
  struct xchar_ent *next;/* hash chain */
} xchar_tab[] =
{
#ifdef USE_UCS4
  {0x309b, XCT_CLOSE, 0}, /* ” */
  {0xff08, XCT_OPEN, 0}, /* （　*/
  {0xff09, XCT_CLOSE, 0}, /* ） */
  {0x3014, XCT_OPEN, 0},  /* 〔 */
  {0x3015, XCT_CLOSE, 0}, /* 〕 */
  {0xff3b, XCT_OPEN, 0}, /* ［ */
  {0xff3d, XCT_CLOSE, 0}, /* ] */
  {0xff5b, XCT_OPEN, 0},  /* { */
  {0xff5d, XCT_CLOSE, 0},  /* ｝　*/
  {0x3008, XCT_OPEN, 0},  /* ＜　*/
  {0x3009, XCT_CLOSE, 0},  /* ＞　*/
  {0x300a, XCT_OPEN, 0},  /* 《　*/
  {0x300b, XCT_CLOSE, 0},  /* 》　*/
  {0x300c, XCT_OPEN, 0},  /* 「　*/
  {0x300d, XCT_CLOSE, 0},  /* 」　*/
  {0x300e, XCT_OPEN, 0},  /* 『　*/
  {0x300f, XCT_CLOSE, 0},  /* 』　*/
  {0x3010, XCT_OPEN, 0},  /* 【　*/
  {0x3011, XCT_CLOSE, 0},  /* 】　*/
#else
  {0xa1c9, XCT_CLOSE, 0}, /* ” */
  {0xa1ca, XCT_OPEN, 0},  /* （　*/
  {0xa1cb, XCT_CLOSE, 0}, /* ） */
  {0xa1cc, XCT_OPEN, 0},  /* 〔 */
  {0xa1cd, XCT_CLOSE, 0}, /* 〕 */
  {0xa1ce, XCT_OPEN, 0}, /* ［ */
  {0xa1cf, XCT_CLOSE, 0}, /* ] */
  {0xa1d0, XCT_OPEN, 0},  /* { */
  {0xa1d1, XCT_CLOSE, 0},  /* ｝　*/
  {0xa1d2, XCT_OPEN, 0},  /* ＜　*/
  {0xa1d3, XCT_CLOSE, 0},  /* ＞　*/
  {0xa1d4, XCT_OPEN, 0},  /* 《　*/
  {0xa1d5, XCT_CLOSE, 0},  /* 》　*/
  {0xa1d6, XCT_OPEN, 0},  /* 「　*/
  {0xa1d7, XCT_CLOSE, 0},  /* 」　*/
  {0xa1d8, XCT_OPEN, 0},  /* 『　*/
  {0xa1d9, XCT_CLOSE, 0},  /* 』　*/
  {0xa1da, XCT_OPEN, 0},  /* 【　*/
  {0xa1db, XCT_CLOSE, 0},  /* 】　*/
#endif
  {28, XCT_OPEN, 0}, /* ( */
  {133, XCT_OPEN, 0}, /* [ */
  {29, XCT_CLOSE, 0}, /* ) */
  {135, XCT_CLOSE, 0}, /* ] */
  {HK_TO, XCT_DEP, 0},/* と */
  {HK_HA, XCT_DEP|XCT_STRONG, 0},/* は */
  {HK_NO, XCT_DEP|XCT_STRONG, 0},/* の */
  {HK_NI, XCT_DEP, 0},/* に */
  {HK_GA, XCT_DEP|XCT_STRONG, 0},/* が */
  {HK_WO, XCT_DEP, 0},/* を */
  {WIDE_0, XCT_WIDENUM, 0},
  {WIDE_1, XCT_WIDENUM, 0},
  {WIDE_2, XCT_WIDENUM, 0},
  {WIDE_3, XCT_WIDENUM, 0},
  {WIDE_4, XCT_WIDENUM, 0},
  {WIDE_5, XCT_WIDENUM, 0},
  {WIDE_6, XCT_WIDENUM, 0},
  {WIDE_7, XCT_WIDENUM, 0},
  {WIDE_8, XCT_WIDENUM, 0},
  {WIDE_9, XCT_WIDENUM, 0},
  {HK_DDOT, XCT_PART, 0},
  {HK_XA, XCT_PART, 0},
  {HK_XI, XCT_PART, 0},
  {HK_XU, XCT_PART, 0},
  {HK_XE, XCT_PART, 0},
  {HK_XO, XCT_PART, 0},
  {HK_XYA, XCT_PART, 0},
  {HK_XYU, XCT_PART, 0},
  {HK_XYO, XCT_PART, 0},
  {HK_TT, XCT_PART, 0},
  {0, 0, 0},
};

static int
find_xchar_type(xchar xc)
{
  struct xchar_ent *xe = xchar_tab;

  for (; xe->xc; xe++) {
    if (xe->xc == xc) {
      return xe->type;
    }
  }

  return XCT_NONE;
}

static int
is_hira(xchar xc)
{
  if ((xc & 0xff00) == 0xa400) {
    return 1;
  }
  if (xc == HK_DDOT) {
    return 1;
  }
  if (xc == HK_BAR) {
    return 1;
  }
  return 0;
}

static int
is_kata(xchar xc)
{
  if ((xc & 0xff00) == 0xa500) {
    return 1;
  }
  return 0;
}

static int
is_symbol(xchar xc)
{
  if ((xc & 0xff00) == 0xa100) {
    return 1;
  }
  if ((xc & 0xff00) == 0xa200) {
    return 1;
  }
  return 0;
}

static int
is_kanji(xchar xc)
{
#ifdef USE_UCS4
  if (xc > 0x4e00 && xc < 0xa000) {
    return 1;
  }
#else
  if (xc > 0xb000 && xc < 0xf400) {
    return 1;
  }
#endif
  return 0;
}

int
anthy_get_xchar_type(xchar xc)
{
  int t = find_xchar_type(xc);
  if (xc > 47 && xc < 58) {
    t |= XCT_NUM;
  }
  if (xc < 128) {
    t |= XCT_ASCII;
  }
  if (is_hira(xc)) {
    t |= XCT_HIRA;
  }
  if (is_kata(xc)) {
    t |= XCT_KATA;
  }
#ifndef USE_UCS4
  if (is_symbol(xc)) {
    if (!(t & XCT_OPEN) && !(t & XCT_CLOSE)) {
      t |= XCT_SYMBOL;
    }
  }
  if (is_kanji(xc)) {
    t |= XCT_KANJI;
  }
#endif
  return t;
}

int
anthy_get_xstr_type(xstr *xs)
{
  int i, t = XCT_ALL;
  for (i = 0; i < xs->len; i++) {
    t &= anthy_get_xchar_type(xs->str[i]);
  }
  return t;
}

int
anthy_xchar_to_num(xchar xc)
{
  switch (xc) {
  case WIDE_0:return 0;
  case WIDE_1:return 1;
  case WIDE_2:return 2;
  case WIDE_3:return 3;
  case WIDE_4:return 4;
  case WIDE_5:return 5;
  case WIDE_6:return 6;
  case WIDE_7:return 7;
  case WIDE_8:return 8;
  case WIDE_9:return 9;
  }
  if (xc >= '0' && xc <= '9') {
    return xc - (int)'0';
  }
  return -1;
}

void
anthy_init_xchar_tab(void)
{
}

/*
 * 品詞の型 wtype_t を扱う
 *
 * 単語の品詞を扱う
 * 単語は次の要素を持つ。
 * *品詞
 * *品詞サブタイプ
 * *品詞サブサブタイプ
 * *活用クラス(CC)
 * *活用形
 * *自立語かどうかのフラグ
 * wtype_tは上記の情報をすべて含む。
 *
 * 各要素の詳細については doc/POSを参照すべし
 */
#ifndef _wtype_h_included_
#define _wtype_h_included_

/*品詞 Part Of Speech */
#define POS_NONE 0
 /* 名詞 */
#define POS_NOUN 1
 /* 助詞(不使用?) */
#define POS_PRT 2
#define POS_XV 3
 /* 動詞 */
#define POS_V 4
 /* 形容詞 */
#define POS_A 5
 /* 形容動詞 */
#define POS_AJV 6
 /* 副詞 */
#define POS_AV 7
 /* 連体詞 */
#define POS_ME 8
 /* 接続詞 */
#define POS_CONJ 9
 /* (不明) */
#define POS_IJ 10
 /* 接頭辞 */
#define POS_PRE 11
 /* 接尾辞 */
#define POS_SUC 12
 /* 単漢字 */
#define POS_TANKANJI 13
 /* 名詞に付く名詞化接尾辞 */
#define POS_N2T 14
 /* 動詞連用形に付く形容詞化接尾辞 */
#define POS_D2KY 15
 /* 数詞 */
#define POS_NUMBER 16
#define POS_INVAL 17
 /* 開き括弧 */
#define POS_OPEN 18
 /* 閉じ括弧 */
#define POS_CLOSE 19

/* 活用形 Conjugate Type */
#define CT_NONE 0
#define CT_SYUSI 1
#define CT_MIZEN 2
#define CT_RENYOU 3
#define CT_RENTAI 4
#define CT_KATEI 5
#define CT_MEIREI 6
 /* 語幹 */
#define CT_HEAD 7

/* 動詞の活用クラス Conjugate Class */
#define CC_NONE 0
 /* カ行五段 */
#define CC_K5 1
 /* カ行五段(行く) */
#define CC_C5 2
 /* ガ行五段 */
#define CC_G5 3
 /* サ行五段 */
#define CC_S5 4
 /* タ行五段 */
#define CC_T5 5
 /* ナ行五段 */
#define CC_N5 6
 /* マ行五段 */
#define CC_M5 7
 /* バ行五段 */
#define CC_B5 8
 /* ラ行五段 */
#define CC_R5 9
 /* ラ行五段(いらっしゃる) */
#define CC_L5 10
 /* ワ行五段 */
#define CC_W5 11
 /* ワ行五段(乞う) */
#define CC_U5 12
 /* 上下一段 */
#define CC_KS1 13
 /* ラ変 */
#define CC_RV 14
 /* カ変 */
#define CC_KV 15
 /* サ変(「する」以外) */
#define CC_SV 16
 /* ザ変 */
#define CC_ZV 17
 /* サ変(する) */
#define CC_SRV 18
 /* カ変と「する」は、活用形の形で辞書に登録されており、かつ、
  * 同じ活用形に複数の読みがあるものがある。それらを区別して
  * 扱うために「その2」を作る。
  * (「副品詞」や「FLAGS」等では区別できなかった) by vagus */
 /* サ変(する) その2  add by vagus */
#define CC_SRV2 19
 /* カ変 その2  add by vagus */
#define CC_KV2 20
 /* 形容動詞 */
#define CC_AJV 21
 /* 形容詞ク活用  add by vagus */
#define CC_A_KU 22
 /* 形容詞シク活用  add by vagus */
#define CC_A_SIKU 23
 /* 形容詞ウ音便 */
#define CC_A_U 24
 /* 形容詞エ音便  add by vagus */
#define CC_A_E 25
 /* 形容詞イ音便  add by vagus */
#define CC_A_I 26
 /* 形容詞「いい」 add by vagus */
#define CC_A_ii 27
 /* 形容詞「ない」 add by vagus */
#define CC_A_nai 28
 /* 形容詞「よい」 add by vagus */
#define CC_A_yoi 29
 /* 連語「非ず」 add by vagus */
#define CC_RZ 30

/* 副品詞 Class Of Speech */
#define COS_NONE 0
 /* 地名 */
#define COS_CN 1
 /* 数詞 */
#define COS_NN 2
 /* 人名 */
#define COS_JN 3
 /* 団体名 */
#define COS_KK 4
 /* 一般接頭辞・接尾辞 */
#define COS_SUFFIX 5
 /* サ変の接尾辞 */
#define COS_SVSUFFIX 6
/**/

/* 副々品詞 Sub Class Of Speech*/
#define SCOS_NONE 0
#define SCOS_FAMNAME 1
#define SCOS_FSTNAME 2
#define SCOS_T0 10
#define SCOS_T1 11
#define SCOS_T2 12
#define SCOS_T3 13
#define SCOS_T4 14
#define SCOS_T5 15
#define SCOS_T6 16
#define SCOS_T7 17
#define SCOS_T8 18
#define SCOS_T9 19
#define SCOS_T10 20
#define SCOS_T11 21
#define SCOS_T12 22
#define SCOS_T13 23
#define SCOS_T14 24
#define SCOS_T15 25
#define SCOS_T16 26
#define SCOS_T17 27
#define SCOS_T18 28
#define SCOS_T19 29
#define SCOS_T20 30
#define SCOS_T21 31
#define SCOS_T22 32
#define SCOS_T23 33
#define SCOS_T24 34
#define SCOS_T25 35
#define SCOS_T26 36
#define SCOS_T27 37
#define SCOS_T28 38
#define SCOS_T29 39
#define SCOS_T30 40
#define SCOS_T31 41
#define SCOS_T32 42
#define SCOS_T33 43
#define SCOS_T34 44
#define SCOS_T35 45
#define SCOS_T36 46
#define SCOS_T37 47
#define SCOS_T38 48
#define SCOS_T39 49
#define SCOS_T40 50
#define SCOS_F0 60
#define SCOS_F1 61
#define SCOS_F2 62
#define SCOS_F3 63
#define SCOS_F4 64
#define SCOS_F5 65
#define SCOS_F6 66
#define SCOS_F7 67
#define SCOS_F8 68
#define SCOS_F9 69
#define SCOS_F10 70
#define SCOS_F11 71
#define SCOS_F12 72
#define SCOS_F13 73
#define SCOS_F14 74
#define SCOS_A0 80
 /* 形容詞化接尾語 */
#define SCOS_A1 81
#define SCOS_N1 90
#define SCOS_N10 91
#define SCOS_N100 92
#define SCOS_N1000 93
#define SCOS_N10000 94

/* FLAGS */
#define WF_NONE 0
 /* この動詞は連用形が名詞化する */
#define WF_MEISI 1
 /* サ変名詞 */
#define WF_SV 2
 /* 自立語、文節のコアとなる */
#define WF_INDEP 4
 /* 形容動詞 */
#define WF_AJV 8

/* wtype_t中のオフセット */
#define WT_POS 0
#define WT_COS 1
#define WT_SCOS 2
#define WT_CC 3
#define WT_CT 4
#define WT_FLAGS 5

/* 各bit fieldの幅 */
#define POS_BITS 5
#define COS_BITS 4
#define SCOS_BITS 7
#define CC_BITS 5
#define CT_BITS 4
#define WF_BITS 4
/* 29bits */

/** 品詞 */
struct wtype{
  unsigned int pos  : POS_BITS;
  unsigned int cos  : COS_BITS;
  unsigned int scos : SCOS_BITS;
  unsigned int cc   : CC_BITS;
  unsigned int ct   : CT_BITS;
  unsigned int wf   : WF_BITS;
};

typedef struct wtype wtype_t;

/* 二つの品詞が完全に一致するかどうかを見る */
int anthy_wtype_equal(wtype_t lhs, wtype_t rhs);

void anthy_print_wtype(wtype_t w);
/* 辞書ファイル中の名前から品詞を得る(関数名が悪い) */
const char *anthy_type_to_wtype(const char *name, wtype_t *w);
/* 品詞の名前から品詞を得る */
wtype_t anthy_init_wtype_by_name(const char *str);

int anthy_wtype_get_pos(wtype_t w);
int anthy_wtype_get_cc(wtype_t w);
int anthy_wtype_get_ct(wtype_t w);
int anthy_wtype_get_cos(wtype_t w);
int anthy_wtype_get_scos(wtype_t w);
int anthy_wtype_get_wf(wtype_t w);

/* フラグの取得 */
int anthy_wtype_get_indep(wtype_t w);
int anthy_wtype_get_sv(wtype_t w);

void anthy_init_wtypes(void);

extern wtype_t anthy_wt_all;/* すべてにマッチする自立語 */
extern wtype_t anthy_wt_none;/* 品詞無しPOS_INVAL */

extern wtype_t anthy_wtype_noun;
extern wtype_t anthy_wtype_num_noun;
extern wtype_t anthy_wtype_a_tail_of_v_renyou;

/* In src-splitter/wordlist.c */
int anthy_init_wordlist (void);
#endif

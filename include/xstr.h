/* 
 * Anthy内部で使う文字列
 * 特に実装を隠蔽しようとしているわけでは無いので、
 * ここにある関数の使用は強制しない。
 */
#ifndef _xstr_h_included_
#define _xstr_h_included_

/** 文字型
 * EUCが入っている */
typedef int xchar;

/** 文字列
 * xstrにtypedefされている
 */
typedef struct xstr_ {
  /** 文字列へのポインタ */
  xchar *str;
  /** xcharの数 */
  int len;
} xstr;

/* デバッグ用の出力関数 */
void anthy_putxchar(xchar );
void anthy_putxstr(xstr *);
void anthy_putxstrln(xstr *);

/* Cの文字列への書き出し */
int anthy_sputxchar(char *, xchar , int encoding);
int anthy_sputxstr(char *, xstr *);
int anthy_snputxstr(char *, int , xstr *, int encoding);

/* xstrとstr共にmallocされる、freeで両方解放するかanthy_free_xstrで解放する */
xstr *anthy_cstr_to_xstr(const char *, int );
/* 結果はmallocで確保される */
char *anthy_xstr_to_cstr(xstr *, int);
xstr *anthy_file_dic_str_to_xstr(const char *);
char *anthy_xstr_to_file_dic_str(xstr *);

/* xstrとstr共にmallocされる */
xstr *anthy_xstr_dup(xstr *);
void anthy_free_xstr(xstr *);

/* 結果はmallocで確保される */
xchar *anthy_xstr_dup_str(xstr *);
void anthy_free_xstr_str(xstr *);

/* 文字列をコピーする */
xstr* anthy_xstrcpy(xstr *, xstr *);
/* 文字列を比較する。strcmpと同等の動作(返り値の符号に意味がある) */
int anthy_xstrcmp(xstr *, xstr *);
/* s->strをreallocする */
xstr *anthy_xstrcat(xstr *s, xstr *d);
/* xs->strをreallocする */
xstr *anthy_xstrappend(xstr *xs, xchar c);

/* strtollのxstr版 */
long long anthy_xstrtoll(xstr *);
/* ひらがなからカタカナへの変換 */
xstr *anthy_xstr_hira_to_kata(xstr *);

/*  xcharの型 */
#define XCT_ALL 0xffffffff
#define XCT_NONE 0
#define XCT_HIRA 1
#define XCT_KATA 2
#define XCT_ASCII 4
#define XCT_NUM 8
#define XCT_WIDENUM 16
#define XCT_OPEN 32
#define XCT_CLOSE 64
/* 直前の文字の一部 */
#define XCT_PART 128
/* 助詞 */
#define XCT_DEP 256
/* 強い接続の付属語文字 */
#define XCT_STRONG 512
/* 記号 */
#define XCT_SYMBOL 1024
/* 漢字 */
#define XCT_KANJI 2048

/** XCT_*が返ってくる */
int anthy_get_xchar_type(xchar );
/** 全ての文字に対してXCT_*の論理積をとったもの */
int anthy_get_xstr_type(xstr *);

/* hash */
int anthy_xstr_hash(xstr *);

/* xstr.c */
int anthy_init_xstr(void);
void anthy_quit_xstr(void);
void anthy_xstr_set_print_encoding(int );
/* おもにfile_dic.cから使用するためのエンコーディング関係の関数
   USE_UCS4が定義されたときだけ実体が存在する */
const char *
anthy_utf8_to_ucs4_xchar(const char *s, xchar *res);


#endif

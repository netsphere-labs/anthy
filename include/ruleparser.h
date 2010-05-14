/*
 * 汎用の設定ファイルパーザ
 */
#ifndef _ruleparser_h_included_
#define _ruleparser_h_included_

/*
 * ファイル名が'/'で始まっていれば絶対パス
 * ファイル名がNULLならば標準入力
 * そうでなければ、ANTHYDIR中のファイルを開ける
 */
int anthy_open_file(const char *fn);
void anthy_close_file(void);
int anthy_read_line(char ***tokens, int *nr);
int anthy_get_line_number(void);
void anthy_free_line(void);

#endif

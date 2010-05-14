/* 学習の履歴などを保存するデータベース */
#ifndef _record_h_included_
#define _record_h_included_
/*
 * データベースは名前をもつ複数のsectionから構成され各セクションは
 * 文字列をキーとして高速に取り出すことができるcolumnからなる。
 *
 * データベースはカレントsectionやカレントcolumnなどの状態を持ち
 * 操作はそれに対して行われる。
 * section中のcolumnは順序関係をもっている
 * その順序関係とは別にLRUの順序をもっている
 */

#include "xstr.h"

/*
 * カレントsectionを設定する
 * name: sectionの名前
 * create_if_not_exist: そのsectionがなければ作るかどうかのフラグ
 * 返り値: 成功 0 、失敗 -1
 * 失敗の時にはカレントsectionは無効になる
 * 常にカレントcolumnは無効になる
 */
int anthy_select_section(const char *name, int create_if_not_exist);

/*
 * カレントsection中からnameのcolumnをカレントcolumnにする
 * name: columnの名前
 * create_if_not_exist: そのcolumnがなければ作るかどうかのフラグ
 * 返り値: 成功 0 、失敗 -1
 * 失敗の時にはカレントcolumnは無効になる
 */
int anthy_select_column(xstr *name, int create_if_not_exist);

/*
 * カレントsection中からnameに最も長い文字数でマッチする
 * 名前のcolumnをカレントcolumnにする
 * name: columnの名前
 * 返り値: 成功 0 、失敗 -1
 * 失敗の時にはにカレントcolumnは無効になる
 */
int anthy_select_longest_column(xstr *name);

/*
 * カレントsection中の最初のcolumnをカレントcolumnにする
 * 返り値: 成功 0 、失敗 -1
 * 失敗の時にはカレントcolumnは無効になる
 */
int anthy_select_first_column(void);

/*
 * カレントcolumnの次のcolumnをカレントcolumnにする
 * 返り値: 成功 0 、失敗 -1
 * カレントcolumnに対する変更があっても、ファイルには保存されない
 * 失敗の時にはカレントcolumnは無効になる
 */
int anthy_select_next_column(void);

/*
 * カレントsectionを解放する
 * 常にカレントsection,columnは無効になる
 */
void anthy_release_section(void);

/*
 * カレントsectionのLRUリストの先頭からcount個以降を解放する
 * 常にカレントcolumnは無効になる
 */
void anthy_truncate_section(int count);


/* 現在のcolumnに対する操作 */
xstr *anthy_get_index_xstr(void);
int anthy_get_nr_values(void);
int anthy_get_nth_value(int );
xstr *anthy_get_nth_xstr(int );/* internされているxstrが返される */

void anthy_set_nth_value(int nth, int val);
void anthy_set_nth_xstr(int nth, xstr *xs);/* 内部でコピーされる */

void anthy_truncate_column(int nth);/* To Be Implemented */

/*
 * カレントcolumnを解放する。終了後のカレントcolumnは不定
 * 常にカレントcolumnは無効になる
 */
void anthy_release_column(void);

/*
 * カレントcolumnをLRUの先頭の方へもってくる
 * 常にカレントcolumnは無効になる
 */
int anthy_mark_column_used(void);


void anthy_reload_record(void);

#endif

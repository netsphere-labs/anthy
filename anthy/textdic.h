#ifndef _textdic_h_included_
#define _textdic_h_included_

int anthy_textdic_scan(const char *fn, long offset, void *ptr,
		       int (*func)(void *, long, const char *, const char *));
int anthy_textdic_insert_line(const char *fn, long offset, const char *line);
int anthy_textdic_delete_line(const char *fn, long offset);

#endif

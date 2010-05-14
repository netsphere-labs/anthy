INCLUDES = -I$(top_srcdir)/include -DCONF_DIR=\"$(sysconfdir)\"

libdiclib_la_SOURCES = \
	diclib.c \
	file_dic.c filemap.c \
	xstr.c xchar.c \
	alloc.c conf.c \
	logger.c \
	ruleparser.c \
	diclib_inner.h


libdiclib_la_LDFLAGS = -version-info 1:0:1
noinst_LTLIBRARIES = libdiclib.la

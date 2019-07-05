AM_CPPFLAGS = -I$(top_srcdir)/

libanthydic_la_SOURCES = \
	word_dic.c dic_util.c \
	wtype.c\
	textdic.c record.c\
	word_lookup.c use_dic.c \
	priv_dic.c mem_dic.c \
	ext_ent.c matrix.c\
	feature_set.c\
	dic_main.h\
	ptab.h wtab.h dic_ent.h \
	mem_dic.h dic_personality.h

libanthydic_la_LIBADD = ../src-diclib/libdiclib.la
libanthydic_la_LDFLAGS = -version-info 2:3:1
lib_LTLIBRARIES = libanthydic.la

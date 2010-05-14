## $Id: Makefile.am,v 1.4 2002/02/25 07:25:03 yusuke Exp $

INCLUDES = -I$(top_srcdir)/include -DCONF_DIR=\"$(sysconfdir)\"

libanthydic_la_SOURCES = \
	dic_main.c dic_util.c\
	alloc.c conf.c ruleparser.c\
	record.c logger.c wtype.c\
	xstr.c xchar.c hash_map.c\
	mem_dic.c file_dic.c use_dic.c\
	dic_session.c ext_ent.c\
	dic_personality.h dic_ent.h mem_dic.h\
	xchar.h dic_main.h\
	ptab.h wtab.h

libanthydic_la_LDFLAGS = -version-info 1:0:1
lib_LTLIBRARIES = libanthydic.la

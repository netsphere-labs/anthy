## $Id: Makefile.am,v 1.10 2002/08/08 07:20:45 gniibe Exp $

# Files
# Dictionary utilities
ELISP_FILES = anthy.el anthy-isearch.el anthy-dic.el anthy-conf.el leim-list.el anthy-azik.el anthy-kyuri.el
EXTRA_DIST = $(ELISP_FILES) typetab dic-tool-usage.txt rcsize.rb
bin_PROGRAMS = anthy-dic-tool anthy-agent
ELCFILES = anthy.elc anthy-dic.elc
if ELISP
lisp_LISP = $(ELISP_FILES)
endif

INCLUDES = -I$(top_srcdir)/include
anthy_dic_tool_SOURCES = dic-tool.c
anthy_dic_tool_LDADD = ../src-diclib/libanthydic.la
anthy_agent_SOURCES = agent.c egg.c
anthy_agent_LDADD =  libanthyinput.la ../src-main/libanthy.la ../src-diclib/libanthydic.la

lib_LTLIBRARIES = libanthyinput.la
libanthyinput_la_SOURCES = input.c rkconv.c rkhelper.c\
 rkconv.h rkmap.h rkhelper.h

pkgdata_DATA = typetab dic-tool-usage.txt

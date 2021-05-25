
SUBDIRS =

# Files
# Dictionary utilities
ELISP_FILES = anthy.el anthy-dic.el anthy-conf.el leim-list.el \
 anthy-isearch.el anthy-azik.el anthy-kyuri.el
EXTRA_DIST = $(ELISP_FILES) typetab dic-tool-usage.txt anthy.i
bin_PROGRAMS = \
    anthy-dic-tool-unicode \
    anthy-agent-unicode \
    anthy-morphological-analyzer-unicode \
    $(NULL)
ELCFILES = anthy.elc anthy-dic.elc anthy-azik.elc anthy-conf.elc anthy-isearch.elc anthy-kyuri.elc leim-list.elc
if ELISP
lisp_LISP = $(ELISP_FILES)
endif

noinst_LTLIBRARIES = libconvdb.la
libconvdb_la_SOURCES = convdb.c

INCLUDES = -I$(top_srcdir)/
anthy_dic_tool_unicode_SOURCES = dic-tool.c
anthy_dic_tool_unicode_LDADD = ../src-worddic/libanthydic-unicode.la
anthy_agent_unicode_SOURCES = agent.c egg.c
anthy_agent_unicode_LDADD = libanthyinput-unicode.la ../src-main/libanthy-unicode.la ../src-worddic/libanthydic-unicode.la
anthy_morphological_analyzer_unicode_SOURCES= morph-main.c
anthy_morphological_analyzer_unicode_LDADD = libconvdb.la ../src-main/libanthy-unicode.la ../src-worddic/libanthydic-unicode.la

lib_LTLIBRARIES = libanthyinput-unicode.la
libanthyinput_unicode_la_SOURCES = input.c rkconv.c rkhelper.c\
 rkconv.h rkmap.h rkhelper.h
libanthyinput_unicode_la_LIBADD = ../src-main/libanthy-unicode.la ../src-worddic/libanthydic-unicode.la

pkgdata_DATA = typetab dic-tool-usage.txt

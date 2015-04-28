AM_CPPFLAGS = -I$(top_srcdir)/

lib_LTLIBRARIES = libanthy.la

libanthy_la_LIBADD = ../src-splitter/libsplit.la ../src-ordering/libordering.la -lm ../src-worddic/libanthydic.la

libanthy_la_LDFLAGS = -version-info 1:0:1

libanthy_la_SOURCES = \
 main.c context.c main.h

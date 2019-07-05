dnl Process this file with autoconf to produce a configure script.
AC_INIT([anthy], [0.4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src-main/main.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AM_PROG_LIBTOOL
AM_PATH_LISPDIR

AM_CONDITIONAL(ELISP, test x$lispdir != x)

dnl without emacs. install-lispLISP does mkdir /anthy
dnl dirty hack to avoid it.
test -n "$lispdir" || lispdir="/tmp/"

lispdir="$lispdir/anthy"

test -z "$GCC" || CFLAGS="$CFLAGS -W -Wall -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -pedantic -Wno-long-long"

AC_OUTPUT(Makefile
 src-diclib/Makefile src-worddic/Makefile 
 src-splitter/Makefile src-ordering/Makefile
 src-main/Makefile src-util/Makefile
 anthy/Makefile
 depgraph/Makefile mkanthydic/Makefile mkworddic/Makefile
 mkworddic/dict.args test/Makefile
 alt-cannadic/Makefile
 doc/Makefile calctrans/Makefile
 anthy-conf anthy-test-conf anthy.spec anthy.pc)

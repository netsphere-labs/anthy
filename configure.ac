#                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_INIT([anthy],[0.4],[hisashi dot horikawa at gmail dot com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src-main/main.c])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
# AM_PROG_LIBTOOL is deprecated.
LT_INIT

AM_PATH_LISPDIR
AM_CONDITIONAL(ELISP, test x$lispdir != x)

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    native_win32=yes
    LDFLAGS="$LDFLAGS -no-undefined"
    ;;
  *)
    native_win32=no
    ;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(OS_WIN32, test "$native_win32" = "yes")

AC_ENABLE_STATIC(no)

dnl without emacs. install-lispLISP does mkdir /anthy
dnl dirty hack to avoid it.
test -n "$lispdir" || lispdir="/tmp"
lispdir="$lispdir/anthy"

# ICE with -fstack-clash-protection. gcc 10.2 on mingw.
# -Wformat=2 includes -Wformat-security
test -z "$GCC" || CFLAGS="$CFLAGS -std=c11 -Wextra -Wall -Wshadow -Wformat=2 -Wformat-truncation -Wundef -fno-common -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wno-unused-parameter -fstack-protector-strong "

AC_CONFIG_FILES([Makefile
 src-diclib/Makefile src-worddic/Makefile 
 src-splitter/Makefile src-ordering/Makefile
 src-main/Makefile src-util/Makefile
 anthy/Makefile
 depgraph/Makefile mkanthydic/Makefile mkworddic/Makefile
 mkworddic/dict.args test/Makefile
 alt-cannadic/Makefile
 doc/Makefile calctrans/Makefile
 anthy-conf anthy-test-conf anthy.spec anthy.pc])
AC_OUTPUT

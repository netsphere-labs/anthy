dnl Process this file with autoconf to produce a configure script.
AC_INIT(src-main/main.c)

AM_INIT_AUTOMAKE(anthy, 8607)
AM_CONFIG_HEADER(config.h)

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

dnl **************************************************************
dnl Extention: Check for SWIG.
dnl **************************************************************
swig_available="no"
AC_ARG_WITH([swig],
            AS_HELP_STRING([--with-swig=PATH],
                           [SWIG path (default: auto-detect)]),
            [RUBY="$withval"])

if test "$SWIG" = "no"; then
  : # ignore
elif test "$SWIG" = ""; then
  AC_PATH_PROG(SWIG, swig, none)
  if test "$swig" != "none"; then
    swig_available="yes"
  fi
else
  AC_CHECK_FILE([$SWIG],
                [swig_available="yes"],
                [AC_MSG_WARN([$SWIG is not found.])])
fi

SWIG_INTERFACE="\$(top_srcdir)/src-util/anthy.i"
SWIG_CPPFLAGS="-I\$(top_srcdir)"
SWIG_OPTIONS="\$(SWIG_CPPFLAGS)"
AC_SUBST(SWIG_INTERFACE)
AC_SUBST(SWIG_CPPFLAGS)
AC_SUBST(SWIG_OPTIONS)

AM_CONDITIONAL([WITH_SWIG], [test "$swig_available" = "yes"])


dnl **************************************************************
dnl Extention: Check for Ruby.
dnl **************************************************************
ruby_available="no"
AC_ARG_WITH([ruby],
            AS_HELP_STRING([--with-ruby=PATH],
                           [Ruby interpreter path [[default=auto-detect]]]),
            [RUBY="$withval"],[RUBY="no"])

if test "$RUBY" = "no"; then
  : # ignore
elif test "$RUBY" = ""; then
  AC_PATH_PROG(RUBY, ruby, none)
  if test "$RUBY" != "none"; then
    ruby_available="yes"
  fi
else
  AC_CHECK_FILE([$RUBY],
                [ruby_available="yes"],
                [AC_MSG_WARN([$RUBY is not found. Disable Ruby extension.])])
fi

if test "$ruby_available" = "yes"; then
  rbconfig="$RUBY -rrbconfig -e "

  changequote(<<, >>)
  for var_name in archdir sitearchdir libdir sitelibdir \
                  CFLAGS LIBRUBYARG DLEXT ; do
    rbconfig_tmp=`$rbconfig "print Config::CONFIG['$var_name']"`
    eval "rbconfig_$var_name=\"$rbconfig_tmp\""
  done
  changequote([, ])

  RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_archdir "
  RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitearchdir "
  RUBY_CFLAGS="$RUBY_CFLAGS $rbconfig_CFLAGS "
  RUBY_LIBS="$rbconfig_LIBRUBYARG"
  RUBY_LDFLAGS="-L$rbconfig_libdir"
  RUBY_DLEXT="$rbconfig_DLEXT"

  RUBY_SITE_LIB_DIR="$rbconfig_sitelibdir"
  RUBY_SITE_ARCH_DIR="$rbconfig_sitearchdir"
  AC_MSG_CHECKING([where to install Ruby scripts])
  AC_ARG_WITH([ruby-site-lib-dir],
	      AS_HELP_STRING([--with-ruby-site-lib-dir=DIR],
                             [Where to install Ruby scripts
                              [[default=auto-detect]]]),
	      [RUBY_SITE_LIB_DIR="$withval"])
  AC_MSG_RESULT([$RUBY_SITE_LIB_DIR])

  AC_MSG_CHECKING([where to install Ruby extensions])
  AC_ARG_WITH([ruby-site-arch-dir],
	      AS_HELP_STRING([--with-ruby-site-arch-dir=DIR],
                             [Where to install Ruby extentions
                              [[default=auto-detect]]]),
                [RUBY_SITE_ARCH_DIR="$withval"])
  AC_MSG_RESULT([$RUBY_SITE_ARCH_DIR])

  AC_SUBST(RUBY_CFLAGS)
  AC_SUBST(RUBY_LIBS)
  AC_SUBST(RUBY_LDFLAGS)
  AC_SUBST(RUBY_DLEXT)
  AC_SUBST(RUBY_SITE_LIB_DIR)
  AC_SUBST(RUBY_SITE_ARCH_DIR)

  _SAVE_CFLAGS=$CFLAGS
  ruby_undef_package_macros="
#undef PACKAGE_NAME
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_VERSION
"
  CFLAGS="$CFLAGS $RUBY_CFLAGS"
  AC_CHECK_HEADERS(ruby.h, [],
                   [ruby_available="no"
                    AC_MSG_WARN([Disable Ruby extension.])],
                   [$ruby_undef_package_macros])
  CFLAGS=$_SAVE_CFLAGS
fi

AM_CONDITIONAL([WITH_RUBY], [test "$ruby_available" = "yes"])


AC_OUTPUT(Makefile
 src-diclib/Makefile src-worddic/Makefile 
 src-splitter/Makefile src-ordering/Makefile
 src-main/Makefile src-util/Makefile src-util/ruby/Makefile
 include/Makefile
 depgraph/Makefile mkanthydic/Makefile mkworddic/Makefile
 mkworddic/dict.args cannadic/Makefile test/Makefile
 doc/Makefile calctrans/Makefile
 anthy-conf anthy-test-conf anthy.spec anthy.pc)

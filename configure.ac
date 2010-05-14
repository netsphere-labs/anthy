dnl Process this file with autoconf to produce a configure script.
AC_INIT(src-main/main.c)

AM_INIT_AUTOMAKE(anthy, 6724)
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

AC_ARG_ENABLE(ucs4,
 AC_HELP_STRING([--enable-ucs4], [use UCS4]), use_ucs="yes")

if test x$use_ucs = xyes; then
AC_DEFINE(USE_UCS4,,[use ucs4])
AC_CHECK_LIB(iconv, iconv)
fi

AC_MSG_CHECKING(for cannadic)
AC_ARG_WITH(cannadic,
  AC_HELP_STRING([--with-cannadic=DIR], [use cannadic in DIR]),
  [test -d "$withval" ||
    AC_MSG_ERROR(specified cannadic directory \`$withval' does not exist)
  case $withval in
    [[\\/$]]* | ?:[[\\/]]* ) ;;	# absolute path is ok
    *) with_cannadic='${top_builddir}/'$withval ;;	# adjust relative path
  esac
  result=$with_cannadic],
  [for result in '${top_builddir}/cannadic' '${top_srcdir}/cannadic' \
		 '${top_builddir}/mkanthydic' '${top_srcdir}/mkanthydic' .; do
    if (top_srcdir=$srcdir; top_builddir=.
        eval test -f \""$result/gcanna.ctd"\" ||
	eval test -f \""$result/gcannaf.ctd"\"); then
       break
    fi
  done
  with_cannadic=$result
  test "$result" = . && result='not found'])
AC_MSG_RESULT($result)
if test "$result/$with_cannadic" = "not found/."; then
  AC_MSG_WARN(cannadic not found; Anthy dictionary will not be built)
fi
CANNADIC_SRCDIR=$with_cannadic
AC_SUBST(CANNADIC_SRCDIR)dnl

AC_OUTPUT(Makefile src-diclib/Makefile src-splitter/Makefile src-ordering/Makefile src-main/Makefile src-util/Makefile include/Makefile depgraph/Makefile mkanthydic/Makefile mkanthydic/dict.args cannadic/Makefile test/Makefile test/test.sh doc/Makefile anthy-conf anthy-test-conf anthy.spec anthy.pc)

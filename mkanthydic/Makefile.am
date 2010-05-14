## $Id: Makefile.am,v 1.10 2002/11/05 15:38:58 yusuke Exp $

# Files
noinst_SCRIPTS =
EXTRA_DICS = base.t extra.t compound.t adjust.t
CANNADIC_DIST = gcanna.ctd gcannaf.ctd
INCLUDES = -I$(top_srcdir)/include
# HOKUTODIC_DIST = hokuto.t
# You can get it from
# http://winnie.kuis.kyoto-u.ac.jp/members/ri/hokuto/dic/index.html
HOKUTODIC_DIST =
CLEANFILES = anthy.dic
EXTRA_DIST = \
 $(EXTRA_DICS) $(HOKUTODIC_DIST) \
 udict

# Generate the dictionary
noinst_PROGRAMS = mkanthydic
mkanthydic_SOURCES = mkdic.c mkudic.c mkdic.h
mkanthydic_LDADD = ../src-diclib/libanthydic.la

anthy.dic : mkanthydic
	@tfiles=; \
	files='$(CANNADIC_DIST)'; for file in $$files; do \
	  case $$file in *.ctd) \
	    if test -f $$file; then tfiles="$$tfiles $$file"; \
	    elif test -f $(CANNADIC_SRCDIR)/$$file; then \
	      tfiles="$$tfiles $(CANNADIC_SRCDIR)/$$file"; fi ;; \
	  esac; done; \
	files='$(EXTRA_DICS)'; for file in $$files; do \
	  case $$file in *.t) \
	    if test -f $$file; then tfiles="$$tfiles $$file"; \
	    elif test -f $(srcdir)/$$file; then \
	      tfiles="$$tfiles $(srcdir)/$$file"; fi ;; \
	  esac; done; \
	if test -z "$$tfiles"; then \
	  rm -f anthy.dic; \
	  echo WARNING: No dictionaries found >&2; \
	else \
	  ./mkanthydic -o anthy.dic $$tfiles -uc $(srcdir)/udict; \
	fi

# To install
pkgdata_DATA = anthy.dic

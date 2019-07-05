AUTOMAKE_OPTIONS =
ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST = COPYING.LIB2_1 \
	README.2007.ja THANKS.2007.ja NEWS.2007.ja INSTALL.2006.ja \
	ChangeLog.2001-2009 ChangeLog.2010 ChangeLog.2015 \
	anthy-conf.in anthy-test-conf.in anthy.spec.in anthy.pc.in Doxyfile \
	corpus/corpus.0.txt corpus/corpus.1.txt corpus/corpus.2.txt \
	corpus/corpus.3.txt corpus/corpus.4.txt corpus/corpus.5.txt

SRCDIR = src-diclib src-worddic src-splitter src-ordering src-main src-util

SUBDIRS = $(SRCDIR) anthy depgraph mkworddic calctrans mkanthydic doc test alt-cannadic

sysconf_DATA = anthy-conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = anthy.pc

MOSTLYCLEANFILES = core

dist-hook: gen-ChangeLog

gen_start_date = 2019-01-01
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d $(srcdir)/.git; then			\
	  log_fix="$(srcdir)/build-aux/git-log-fix";			\
	  test -e "$$log_fix"						\
	    && amend_git_log="--amend=$$log_fix"			\
	    || amend_git_log=;						\
	  $(top_srcdir)/build-aux/gitlog-to-changelog			\
	    $$amend_git_log --since=$(gen_start_date)			\
	    --ignore-matching '^;' > $(distdir)/cl-t &&			\
	    { rm -f $(distdir)/ChangeLog &&				\
	      mv $(distdir)/cl-t $(distdir)/ChangeLog; }		\
	fi

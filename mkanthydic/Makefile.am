## $Id: Makefile.am,v 1.10 2002/11/05 15:38:58 yusuke Exp $

# Files
noinst_SCRIPTS =
EXTRA_DICS = base.t extra.t compound.t name.t adjust.t katakana.t
CANNADIC_DIST = gcanna.ctd gcannaf.ctd
INCLUDES = -I$(top_srcdir)/include
# HOKUTODIC_DIST = hokuto.t
# You can get it from
# http://winnie.kuis.kyoto-u.ac.jp/members/ri/hokuto/dic/index.html
HOKUTODIC_DIST =
CLEANFILES = anthy.dic
EXTRA_DIST = \
 $(EXTRA_DICS) $(HOKUTODIC_DIST) \
 udict dict.args.in

# Generate the dictionary
noinst_PROGRAMS = mkanthydic
mkanthydic_SOURCES = mkdic.c writewords.c mkudic.c mkdic.h
mkanthydic_LDADD = ../src-diclib/libanthydic.la

anthy.dic : mkanthydic
	  ./mkanthydic -f ./dict.args


# To install
pkgdata_DATA = anthy.dic

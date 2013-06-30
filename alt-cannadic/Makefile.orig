#
# Makefile
# 
# Copyright Masao SUGIMOTO. 2000, 2001, 2002, 2003
#
# Author: Masao SUGIMOTO <sugimoto@oucrc.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Emacs; see the file COPYING.  If not, write to the
# Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

MAINTXTDIC      = gcanna.ctd
SUBTXTDIC       = gcannaf.ctd

all: help

help:
	@echo "see README"

maindic:
	crxdic -m -o ${MAINTXTDIC:.ctd=.cbd} ${MAINTXTDIC}
	crfreq -div 512 ${MAINTXTDIC:.ctd=.cbd} ${MAINTXTDIC:.ctd=.mwd}

maindic-old-version:
	perl convert-old-cannadic.pl
	crxdic -m -o ${MAINTXTDIC:.ctd=.cbd}.orig ${MAINTXTDIC}
	crfreq ${MAINTXTDIC:.ctd=.cbd} ${MAINTXTDIC:.ctd=.mwd}

subdic:
	crxdic -s -o ${SUBTXTDIC:.ctd=.cbd} ${SUBTXTDIC}
	crfreq ${SUBTXTDIC:.ctd=.cbd} ${SUBTXTDIC:.ctd=.swd}

# GNU Mailutils -- a suite of utilities for electronic mail
# Copyright (C) 2010-2012 Free Software Foundation, Inc.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

mu_aux_dir = ../../mu-aux

VPATH = ../../libmailutils/tests/:../../examples/
.SUFFIXES: .c .inc
.c.inc:
	sed -f $(mu_aux_dir)/texify.sed $< > $@

all: addr.inc http.inc mailcap.inc numaddr.inc sfrom.inc url-parse.inc

addr.inc:	addr.c
http.inc:	http.c
mailcap.inc:	mailcap.c
numaddr.inc:	numaddr.c
sfrom.inc:	sfrom.c
url-parse.inc:	url-parse.c

clean:;	rm -f *.inc

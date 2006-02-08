## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
##   your option) any later version.
##
##   This program is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@ @MU_AUTHINCS@
bin_PROGRAMS = movemail
AM_CPPFLAGS = -DGNULIB_ARGP_EXTERN_BASENAME=1

movemail_LDADD = \
 ../lib/libmuaux.la\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/nntp/libmu_nntp.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 @MU_AUTHLIBS@\
 ../mailbox/libmailutils.la\
 @MU_COMMON_LIBRARIES@

CFLOW_FLAGS=-i^s --brief -v
movemail_CFLOW_INPUT=$(movemail_OBJECTS:.$(OBJEXT)=.c)
movemail.cflow: $(movemail_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
	$(movemail_CFLOW_INPUT)

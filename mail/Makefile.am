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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AM_YFLAGS=-vt
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib -I${top_srcdir}/mailbox @INTLINCS@ @AUTHINCS@ 
AM_CFLAGS = -DSITE_MAIL_RC=\"@SITE_MAIL_RC@\"

bin_PROGRAMS = mail

mail_LDADD = \
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la\
 @AUTHLIBS@ @READLINE_LIBS@ @LTLIBINTL@

EXTRA_DIST = msgset.c

mail_SOURCES = alias.c alt.c cd.c copy.c delete.c dp.c echo.c \
  edit.c eq.c exit.c file.c folders.c followup.c from.c headers.c help.c \
  hold.c if.c inc.c list.c mail.c mail.h mailline.c mbox.c msgset.y next.c \
  pipe.c previous.c print.c quit.c reply.c retain.c save.c send.c set.c \
  shell.c size.c source.c summary.c table.c tag.c top.c touch.c unalias.c \
  undelete.c unset.c util.c var.c version.c visual.c write.c z.c decode.c \
  setenv.c

SUBDIRS = testsuite

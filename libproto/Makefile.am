## This file is part of GNU Mailutils.
## Copyright (C) 2006-2007, 2009-2012 Free Software Foundation, Inc.
##
## GNU Mailutils is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 3, or (at
## your option) any later version.
##
## GNU Mailutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. 

if MU_COND_SUPPORT_POP
  POP_DIR = pop
endif

if MU_COND_SUPPORT_IMAP
  IMAP_DIR = imap
endif

if MU_COND_SUPPORT_NNTP
  NNTP_DIR = nntp
endif

if MU_COND_SUPPORT_MH
  MH_DIR = mh
endif

if MU_COND_SUPPORT_MAILDIR
  MAILDIR_DIR = maildir
endif

SUBDIRS = mbox $(POP_DIR) $(NNTP_DIR) $(IMAP_DIR) $(MH_DIR) $(MAILDIR_DIR) mailer


# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include 

sbin_PROGRAMS = comsatd

comsatd_SOURCES = action.c cfg.c comsat.c comsat.h

comsatd_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la 

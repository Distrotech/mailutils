/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _READMSG_H
#define _READMSG_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/mailbox.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/filter.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/url.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>
#include <mailutils/error.h>
#include <mailutils/envelope.h>
#include <mailutils/wordsplit.h>
#include <mailutils/datetime.h>

int msglist (mu_mailbox_t mbox, int show_all, int argc, char **argv, int **set, int *n);

#endif


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2011-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_PROG_STREAM_H
#define _MAILUTILS_SYS_PROG_STREAM_H

#include <mailutils/sys/stream.h>
#include <mailutils/prog.h>

struct _mu_prog_stream
{
  struct _mu_stream stream;   /* Base stream */
  char *progname;             /* Program name */
  size_t argc;                /* Number of arguments */
  char **argv;                /* Program arguments */
  int hint_flags;             /* Hint flags */
  struct mu_prog_hints hints; /* Invocation hints */
  
  pid_t pid;
  int status;
  pid_t writer_pid;
  
  mu_stream_t in, out;
};

extern int _mu_prog_limit_flags;
extern int _mu_prog_limit_codes[_MU_PROG_LIMIT_MAX];

#endif

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2011-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_PROG_H
#define _MAILUTILS_PROG_H

#include <sys/time.h>
#include <sys/resource.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_PROG_LIMIT_AS      0
#define MU_PROG_LIMIT_CPU     1
#define MU_PROG_LIMIT_DATA    2
#define MU_PROG_LIMIT_FSIZE   3
#define MU_PROG_LIMIT_NPROC   4
#define MU_PROG_LIMIT_CORE    5
#define MU_PROG_LIMIT_MEMLOCK 6
#define MU_PROG_LIMIT_NOFILE  7
#define MU_PROG_LIMIT_RSS     8
#define MU_PROG_LIMIT_STACK   9

#define _MU_PROG_LIMIT_MAX   10

#define MU_PROG_HINT_WORKDIR     0x0001  /* Workdir is set */
#define MU_PROG_HINT_PRIO        0x0002  /* Prio is set */
#define MU_PROG_HINT_INPUT       0x0004  /* Input stream is set */
#define MU_PROG_HINT_UID         0x0008  /* Uid is set */
#define MU_PROG_HINT_GID         0x0010  /* Supplementary gids are set */
#define MU_PROG_HINT_ERRTOOUT    0x0020  /* Redirect stderr to stdout */
#define MU_PROG_HINT_ERRTOSTREAM 0x0040  /* Redirect stderr to errstream */
#define MU_PROG_HINT_IGNOREFAIL  0x0080  /* Ignore hint setup failures */
#define _MU_PROG_HINT_MASK       0x00ff
#define MU_PROG_HINT_LIMIT(n) (0x100 << (n)) /* MU_PROG_LIMIT_n is set */

struct mu_prog_hints
{
  char *mu_prog_workdir;                    /* Working directory */
  uid_t mu_prog_uid;                        /* Run as this user */
  gid_t *mu_prog_gidv;                      /* Array of supplementary gids */
  size_t mu_prog_gidc;                      /* Number of elements in gidv */
  rlim_t mu_prog_limit[_MU_PROG_LIMIT_MAX]; /* Limits */
  int mu_prog_prio;                         /* Scheduling priority */
  mu_stream_t mu_prog_input;                /* Input stream */
  mu_stream_t mu_prog_error;                /* Error stream */       
};

int mu_prog_stream_create (mu_stream_t *pstream,
			   const char *progname,
			   size_t argc, char **argv,
			   int hflags,
			   struct mu_prog_hints *hints,
			   int flags);
int mu_command_stream_create (mu_stream_t *pstream, const char *command,
			      int flags);

#ifdef __cplusplus
}
#endif

#endif


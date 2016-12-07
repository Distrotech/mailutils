/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2016 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_GLOB_H
#define _MAILUTILS_GLOB_H

#include <mailutils/types.h>
#include <regex.h>

/* Produce case-insensitive regex */
#define MU_GLOBF_ICASE    0x01
/* Treat each wildcard as regexp parenthesized group */
#define MU_GLOBF_SUB      0x02
/* When used with MU_GLOBF_SUB - collapse contiguous runs of * to single
   asterisk */
#define MU_GLOBF_COLLAPSE 0x04

int mu_glob_to_regex_opool (char const *pattern, mu_opool_t pool, int flags);
int mu_glob_to_regex (char **rxstr, char const *pattern, int flags);
int mu_glob_compile (regex_t *rx, char const *pattern, int flags);

#endif

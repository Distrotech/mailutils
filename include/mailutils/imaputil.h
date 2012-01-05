/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_IMAPUTIL_H
# define _MAILUTILS_IMAPUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

# include <mailutils/types.h>

int mu_imap_wildmatch (const char *pattern, const char *name, int delim);

int mu_imap_flag_to_attribute (const char *item, int *attr);
  int mu_imap_format_flags (mu_stream_t str, int flags, int include_recent);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_IMAPUTIL_H */

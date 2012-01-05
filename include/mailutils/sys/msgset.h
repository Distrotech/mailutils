/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_MSGSET_H
# define _MAILUTILS_SYS_MSGSET_H

#define _MU_MSGSET_AGGREGATED 0x10
#define _MU_MSGSET_USERFLAG_MASK 0x0f

struct _mu_msgset
{
  mu_list_t list;           /* List of mu_msgrange structures */
  mu_mailbox_t mbox;        /* Associated mailbox */
  int flags;                /* Message set flags */
};

int _mu_msgset_translate_pair (mu_msgset_t mset, int mode,
			       size_t *beg, size_t *end);
int _mu_msgset_translate_range (mu_msgset_t mset, int mode,
				struct mu_msgrange *r);

#endif

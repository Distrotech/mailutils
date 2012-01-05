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

#include <config.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/msgset.h>

/* Comparator function used to locate a message in the list.  Second argument
   (b) is a pointer to the message number. */
static int
compare_msgnum (const void *a, const void *b)
{
  struct mu_msgrange const *range = a;
  size_t msgno = *(size_t*)b;

  if (range->msg_beg <= msgno && msgno <= range->msg_end)
    return 0;
  return 1;
}

int
mu_msgset_create (mu_msgset_t *pres, mu_mailbox_t mbox, int flags)
{
  mu_msgset_t msgset;
  int rc;

  msgset = calloc (1, sizeof (*msgset));
  if (!msgset)
    return ENOMEM;
  
  rc = mu_list_create (&msgset->list);
  if (rc)
    {
      free (msgset);
      return rc;
    }
  mu_list_set_destroy_item (msgset->list, mu_list_free_item);
  mu_list_set_comparator (msgset->list, compare_msgnum);
  msgset->mbox = mbox;
  msgset->flags = flags & _MU_MSGSET_USERFLAG_MASK;
  *pres = msgset;
  return 0;
}

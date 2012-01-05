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
#include <mailutils/mailbox.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/msgset.h>

struct invert_closure
{
  mu_msgset_t nset;
  size_t next_num;
};

static int
_invert_range (void *item, void *data)
{
  struct mu_msgrange *range = item;
  struct invert_closure *clos = data;
  int rc;
  
  if (clos->next_num < range->msg_beg - 1)
    {
      rc = mu_msgset_add_range (clos->nset,
				clos->next_num, range->msg_beg - 1,
				clos->nset->flags);
      if (rc)
	return rc;
    }
  clos->next_num = range->msg_end + 1;
  return 0;
}

/* Negate the message set: on return PNSET consists of the messages
   _not contained_ in the input message set. */
int
mu_msgset_negate (mu_msgset_t msgset, mu_msgset_t *pnset)
{
  int rc;
  struct invert_closure clos;
  size_t total;
  
  if (!msgset)
    return EINVAL;
  if (!msgset->mbox)
    return MU_ERR_NOT_OPEN;

  rc = mu_msgset_aggregate (msgset);
  if (rc)
    return rc;
  rc = mu_mailbox_messages_count (msgset->mbox, &total);
  if (rc)
    return rc;
  if (msgset->flags == MU_MSGSET_UID)
    {
      rc = mu_mailbox_translate (msgset->mbox,
				 MU_MAILBOX_MSGNO_TO_UID,
				 total, &total);
      if (rc)
	return rc;
    }
  rc = mu_msgset_create (&clos.nset, msgset->mbox, msgset->flags);
  if (rc)
    return rc;
  clos.next_num = 1;
  rc = mu_list_foreach (msgset->list, _invert_range, &clos);
  if (rc == 0)
    {
      if (clos.next_num < total)
	rc = mu_msgset_add_range (clos.nset, clos.next_num, total,
				  clos.nset->flags);
    }
  
  if (rc)
    mu_msgset_free (clos.nset);
  else
    *pnset = clos.nset;

  return rc;
}

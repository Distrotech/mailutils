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
#include <mailutils/mailbox.h>
#include <mailutils/sys/msgset.h>

struct action_closure
{
  mu_msgset_message_action_t action;
  void *data;
  mu_msgset_t msgset;
  int dir;
};

static int
call_action (struct action_closure *clos, size_t i)
{
  int rc;
  mu_message_t msg = NULL;
  size_t n;

  if (clos->msgset->flags == MU_MSGSET_UID)
    {
      rc = mu_mailbox_translate (clos->msgset->mbox, MU_MAILBOX_UID_TO_MSGNO,
				 i, &n);
      if (rc == MU_ERR_NOENT)
	return 0;
      else if (rc)
	return rc;
    }
  else
    n = i;
  
  rc = mu_mailbox_get_message (clos->msgset->mbox, n, &msg);
  if (rc == MU_ERR_NOENT)
    return 0;
  else if (rc == 0)
    rc = clos->action (i, msg, clos->data);
  return rc;
}

static int
procrange (void *item, void *data)
{
  struct mu_msgrange *mp = item;
  struct action_closure *clos = data;
  size_t i;
  int rc = 0;
  
  if (clos->dir)
    for (i = mp->msg_end; rc == 0 && i >= mp->msg_beg; i--)
      rc = call_action (clos, i);
  else
    for (i = mp->msg_beg; i <= mp->msg_end; i++)
      rc = call_action (clos, i);
  return rc;
}

/* Apply ACTION to each message number from MSGSET. */
int
mu_msgset_foreach_dir_message (mu_msgset_t msgset, int dir,
			       mu_msgset_message_action_t action,
			       void *data)
{
  int rc;
  struct action_closure clos;

  if (!msgset->mbox)
    return MU_ERR_NOT_OPEN;
  rc = mu_msgset_aggregate (msgset);
  if (rc)
    return rc;
  clos.action = action;
  clos.data = data;
  clos.msgset = msgset;
  clos.dir = dir;
  return mu_list_foreach_dir (msgset->list, dir, procrange, &clos);
}

int
mu_msgset_foreach_message (mu_msgset_t msgset,
			   mu_msgset_message_action_t action,
			   void *data)
{
  return mu_msgset_foreach_dir_message (msgset, 0, action, data);
}

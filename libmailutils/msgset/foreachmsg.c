/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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
procrange (void *item, void *data)
{
  struct mu_msgrange *mp = item;
  struct action_closure *clos = data;
  size_t i;

  if (clos->dir)
    for (i = mp->msg_end; i >= mp->msg_beg; i--)
      {
	int rc;
	mu_message_t msg = NULL;
	
	if (clos->msgset->mbox)
	  {
	    rc = mu_mailbox_get_message (clos->msgset->mbox, i, &msg);
	    if (rc == MU_ERR_NOENT)
	      continue;
	    else if (rc)
	    return rc;
	  }
	rc = clos->action (i, msg, clos->data);
	if (rc)
	  return rc;
      }
  else
    for (i = mp->msg_beg; i <= mp->msg_end; i++)
      {
	int rc;
	mu_message_t msg = NULL;
	
	if (clos->msgset->mbox)
	  {
	    rc = mu_mailbox_get_message (clos->msgset->mbox, i, &msg);
	    if (rc == MU_ERR_NOENT)
	      continue;
	    else if (rc)
	      return rc;
	  }
	rc = clos->action (i, msg, clos->data);
	if (rc)
	  return rc;
      }
  return 0;
}

/* Apply ACTION to each message number from MSGSET. */
int
mu_msgset_foreach_dir_message (mu_msgset_t msgset, int dir,
			       mu_msgset_message_action_t action,
			       void *data)
{
  int rc;
  struct action_closure clos;
  
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

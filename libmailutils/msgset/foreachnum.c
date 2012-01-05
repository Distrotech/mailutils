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
  mu_msgset_msgno_action_t action;
  void *data;
  mu_msgset_t msgset;
  int flags;
};

static int
call_action (struct action_closure *clos, size_t i)
{
  size_t n;
  int cmd;
  
  if (clos->msgset->flags != (clos->flags & MU_MSGSET_MODE_MASK))
    {
      int rc;

      switch (clos->flags & MU_MSGSET_MODE_MASK)
	{
	case MU_MSGSET_NUM:
	  cmd = MU_MAILBOX_UID_TO_MSGNO;
	  break;
	  
	case MU_MSGSET_UID:
	  cmd = MU_MAILBOX_MSGNO_TO_UID;
	  break;
	  
	default:
	  return EINVAL;
	}
      
      rc = mu_mailbox_translate (clos->msgset->mbox, cmd, i, &n);
      if (rc == MU_ERR_NOENT)
	return 0;
      if (rc)
	return rc;
    }
  else
    n = i;
  return clos->action (n, clos->data);
}
    

static int
procrange (void *item, void *data)
{
  struct mu_msgrange *mp = item;
  struct action_closure *clos = data;
  size_t i;
  int rc = 0;
  
  if (clos->flags & MU_MSGSET_FOREACH_BACKWARD)
    for (i = mp->msg_end; rc == 0 && i >= mp->msg_beg; i--)
      rc = call_action (clos, i);
  else
    for (i = mp->msg_beg; rc == 0 && i <= mp->msg_end; i++)
      rc = call_action (clos, i);
  return rc;
}

/* Apply ACTION to each message number or UID from MSGSET. */
int
mu_msgset_foreach_num (mu_msgset_t msgset, int flags,
		       mu_msgset_msgno_action_t action,
		       void *data)
{
  int rc;
  struct action_closure clos;
  
  rc = mu_msgset_aggregate (msgset);
  if (rc)
    return rc;
  clos.action = action;
  clos.data = data;
  clos.flags = flags;
  clos.msgset = msgset;
  return mu_list_foreach_dir (msgset->list,
			      !!(flags & MU_MSGSET_FOREACH_BACKWARD),
			      procrange, &clos);
}

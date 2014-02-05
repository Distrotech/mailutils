/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2014 Free Software Foundation, Inc.

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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/msgset.h>
#include <mailutils/nls.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

struct search_closure
{
  mu_msgset_t msgset;
  int retcode;
};

static int
add_msgno (void *item, void *data)
{
  int rc;
  struct imap_list_element *elt = item;
  struct search_closure *scp = data;
  char *p;
  unsigned long num;
  
  if (elt->type != imap_eltype_string)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		(_("unexpected list element in untagged response from SEARCH")));
      scp->retcode = MU_ERR_BADREPLY;
      return 1;
    }

  if (!scp->msgset)
    {
      /* First element (SEARCH): create the message set and return */
      rc = mu_msgset_create (&scp->msgset, NULL, MU_MSGSET_NUM);
      if (rc)
	{
	  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		    (_("cannot create msgset: %s"),
		     mu_strerror (rc)));
	  scp->retcode = rc;
	  return rc;
	}
      return 0;
    }
  
  num = strtoul (elt->v.string, &p, 10);
  if (*p)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		(_("not a number in untagged response from SEARCH: %s"),
		 elt->v.string));
      scp->retcode = MU_ERR_BADREPLY;
      return 1;
    }
  
  rc = mu_msgset_add_range (scp->msgset, num, num, MU_MSGSET_NUM);
  if (rc)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		("mu_msgset_add_range: %s", mu_strerror (rc)));
      scp->retcode = rc;
      return 1;
    }
  return 0;
}

static void
search_handler (mu_imap_t imap, mu_list_t resp, void *data)
{
  mu_list_foreach (resp, add_msgno, data);
}

int
mu_imap_search (mu_imap_t imap, int uid, const char *expr, mu_msgset_t *msgset)
{
  char const *argv[3];
  int i, rc;
  static struct imap_command com;
  struct search_closure clos;

  if (!expr)
    return EINVAL;
  if (!msgset)
    return MU_ERR_OUT_PTR_NULL;
  i = 0;
  if (uid)
    argv[i++] = "UID";
  argv[i++] = "SEARCH";

  clos.msgset = NULL;
  clos.retcode = 0;
  
  com.session_state = MU_IMAP_SESSION_SELECTED;
  com.capa = NULL;
  com.rx_state = MU_IMAP_CLIENT_SEARCH_RX;
  com.argc = i;
  com.argv = argv;
  com.extra = expr;
  com.msgset = NULL;
  com.tagged_handler = NULL;
  com.untagged_handler = search_handler;
  com.untagged_handler_data = &clos;
  rc = mu_imap_gencom (imap, &com);
  if (rc)
      mu_msgset_free (clos.msgset);
  else if (clos.retcode)
    {
      mu_msgset_free (clos.msgset);
      rc = clos.retcode;
    }
  else
    *msgset = clos.msgset;
  return rc;
}

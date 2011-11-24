/* Response processing for IMAP client.
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/cstr.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>

struct mu_kwd mu_imap_response_codes[] = {
  /* [ALERT] */
  { "ALERT",          MU_IMAP_RESPONSE_ALERT },
  /* [BADCHARSET (opt-list)] */
  { "BADCHARSET",     MU_IMAP_RESPONSE_BADCHARSET },
  /* [CAPABILITY (list)] */
  { "CAPABILITY",     MU_IMAP_RESPONSE_CAPABILITY },
  /* [PARSE] text */
  { "PARSE",          MU_IMAP_RESPONSE_PARSE }, 
  /* [PERMANENTFLAGS (list)] */
  { "PERMANENTFLAGS", MU_IMAP_RESPONSE_PERMANENTFLAGS },
  /* [READ-ONLY] */
  { "READ-ONLY",      MU_IMAP_RESPONSE_READ_ONLY },
  /* [READ-WRITE] */
  { "READ-WRITE",     MU_IMAP_RESPONSE_READ_WRITE },
  /* [TRYCREATE] */
  { "TRYCREATE",      MU_IMAP_RESPONSE_TRYCREATE },  
  /* [UIDNEXT N] */
  { "UIDNEXT",        MU_IMAP_RESPONSE_UIDNEXT }, 
  /* [UIDVALIDITY N] */
  { "UIDVALIDITY",    MU_IMAP_RESPONSE_UIDVALIDITY },
  /* [UNSEEN N] */
  { "UNSEEN",         MU_IMAP_RESPONSE_UNSEEN },
  { NULL }
};

static void
ok_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  struct imap_list_element *arg;
  int rcode = -1;
  size_t n = 0;
  
  arg = _mu_imap_list_at (resp, 1);
  if (!arg)
    return;
  if (_mu_imap_list_element_is_string (arg, "["))
    {
      char *p;
      
      arg = _mu_imap_list_at (resp, 2);
      if (!arg || arg->type != imap_eltype_string)
	return;
      
      if (mu_kwd_xlat_name (mu_imap_response_codes, arg->v.string, &rcode))
	rcode = -1;
      
      arg = _mu_imap_list_at (resp, 4);
      if (!arg || !_mu_imap_list_element_is_string (arg, "]"))
	return;
      
      switch (rcode)
	{
	case MU_IMAP_RESPONSE_PERMANENTFLAGS:
	  arg = _mu_imap_list_at (resp, 3);
	  if (!arg ||
	      _mu_imap_collect_flags (arg, &imap->mbox_stat.permanent_flags))
	    break;
	  imap->mbox_stat.flags |= MU_IMAP_STAT_PERMANENT_FLAGS;
	  mu_imap_callback (imap, MU_IMAP_CB_PERMANENT_FLAGS, resp,
			    &imap->mbox_stat);
	  return;
	  
	case MU_IMAP_RESPONSE_UIDNEXT:
	  arg = _mu_imap_list_at (resp, 3);
	  if (!arg || arg->type != imap_eltype_string)
	    break;
	  n = strtoul (arg->v.string, &p, 10);
	  if (*p == 0)
	    {
	      imap->mbox_stat.uidnext = n;
	      imap->mbox_stat.flags |= MU_IMAP_STAT_UIDNEXT;
	      mu_imap_callback (imap, MU_IMAP_CB_UIDNEXT, resp,
				&imap->mbox_stat);
	    }
	  return;
			    
	case MU_IMAP_RESPONSE_UIDVALIDITY:
	  arg = _mu_imap_list_at (resp, 3);
	  if (!arg || arg->type != imap_eltype_string)
	    break;
	  n = strtoul (arg->v.string, &p, 10);
	  if (*p == 0)
	    {
	      imap->mbox_stat.uidvalidity = n;
	      imap->mbox_stat.flags |= MU_IMAP_STAT_UIDVALIDITY;
	      mu_imap_callback (imap, MU_IMAP_CB_UIDVALIDITY, resp,
				&imap->mbox_stat);
	    }
	  return;
			    
	case MU_IMAP_RESPONSE_UNSEEN:
	  arg = _mu_imap_list_at (resp, 3);
	  if (!arg || arg->type != imap_eltype_string)
	    break;
	  n = strtoul (arg->v.string, &p, 10);
	  if (*p == 0)
	    {
	      imap->mbox_stat.first_unseen = n;
	      imap->mbox_stat.flags |= MU_IMAP_STAT_FIRST_UNSEEN;
	      mu_imap_callback (imap, MU_IMAP_CB_FIRST_UNSEEN, resp,
				&imap->mbox_stat);
	    }
	  return;
	}
    }
  mu_imap_callback (imap, MU_IMAP_CB_OK, resp, rcode);
}


struct response_closure
{
  mu_imap_t imap;
  mu_imap_response_action_t fun;
  void *data;
};

struct resptab
{
  char *name;
  mu_imap_response_action_t action;
};

static struct resptab resptab[] = {
  { "OK", ok_response },
  { "NO", },
  { "BAD", },
  { "PREAUTH", },
  { "BYE", },
  { NULL }
};

static int
_std_unsolicited_response (mu_imap_t imap, size_t count, mu_list_t resp)
{
  struct resptab *rp;
  struct imap_list_element *arg = _mu_imap_list_at (resp, 0);

  if (!arg)
    return 1;

  if (arg->type == imap_eltype_string)
    for (rp = resptab; rp->name; rp++)
      {
	if (mu_c_strcasecmp (rp->name, arg->v.string) == 0)
	  {
	    if (!rp->action)
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE9,
			("%s:%d: ignoring %s response",
			 __FILE__, __LINE__, rp->name));
	    else
	      rp->action (imap, resp, NULL);
	    return 0;
	  }
      }
  return 1;
}

static int
_process_unsolicited_response (mu_imap_t imap, mu_list_t resp)
{
  size_t count;
  struct imap_list_element *arg;
  
  if (mu_list_count (resp, &count))
    return 1;

  if (_std_unsolicited_response (imap, count, resp) == 0)
    return 0;
  if (count == 2)
    {
      size_t n;
      char *p;

      arg = _mu_imap_list_at (resp, 1);
      if (!arg)
	return 1;
      
      if (_mu_imap_list_element_is_string (arg, "EXISTS"))
	{
	  arg = _mu_imap_list_at (resp, 0);
	  if (!arg)
	    return 1;
	  
	  n = strtoul (arg->v.string, &p, 10);
	  if (*p)
	    return 1;
	  imap->mbox_stat.message_count = n;
	  imap->mbox_stat.flags |= MU_IMAP_STAT_MESSAGE_COUNT;
	  mu_imap_callback (imap, MU_IMAP_CB_MESSAGE_COUNT, resp, n);
	  return 0;
	}
      else if (_mu_imap_list_element_is_string (arg, "RECENT"))
	{
	  arg = _mu_imap_list_at (resp, 0);
	  if (!arg)
	    return 1;
	  n = strtoul (arg->v.string, &p, 10);
	  if (*p)
	    return 1;
	  imap->mbox_stat.recent_count = n;
	  imap->mbox_stat.flags |= MU_IMAP_STAT_RECENT_COUNT;
	  mu_imap_callback (imap, MU_IMAP_CB_RECENT_COUNT, resp, n);
	  return 0;
	}
    }
  return 1;
}

static int
_process_response (void *item, void *data)
{
  struct imap_list_element *elt = item;
  struct response_closure *clos = data;

  if (elt->type != imap_eltype_list)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		("ignoring string response \"%s\"", elt->v.string));
    }
  else if (_process_unsolicited_response (clos->imap, elt->v.list))
    clos->fun (clos->imap, elt->v.list, clos->data);
  return 0;
}

int
mu_imap_foreach_response (mu_imap_t imap, mu_imap_response_action_t fun,
			  void *data)
{
  struct response_closure clos;
  clos.imap = imap;
  clos.fun = fun;
  clos.data = data;
  return mu_list_foreach (imap->untagged_resp, _process_response, &clos);
}
			  

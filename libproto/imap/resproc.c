/* Response processing for IMAP client.
   GNU Mailutils -- a suite of utilities for electronic mail
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

static int
parse_response_code (mu_imap_t imap, mu_list_t resp)
{
  struct imap_list_element *arg;
  int rcode = -1;

  arg = _mu_imap_list_at (resp, 1);
  if (!arg)
    return -1;
  
  if (_mu_imap_list_element_is_string (arg, "["))
    {
      arg = _mu_imap_list_at (resp, 2);
      if (!arg || arg->type != imap_eltype_string)
	return -1;
      
      if (mu_kwd_xlat_name (mu_imap_response_codes, arg->v.string, &rcode))
	return -1;
      
      arg = _mu_imap_list_at (resp, 3);
      if (!arg || !_mu_imap_list_element_is_string (arg, "]"))
	return -1;
    }
  return rcode;
}
  
static void
ok_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  struct imap_list_element *arg;
  int rcode;
  size_t n = 0;
  char *p;

  rcode = parse_response_code (imap, resp);
  switch (rcode)
    {
    case MU_IMAP_RESPONSE_PERMANENTFLAGS:
      arg = _mu_imap_list_at (resp, 3);
      if (!arg ||
	  _mu_imap_collect_flags (arg, &imap->mbox_stat.permanent_flags))
	break;
      imap->mbox_stat.flags |= MU_IMAP_STAT_PERMANENT_FLAGS;
      mu_imap_callback (imap, MU_IMAP_CB_PERMANENT_FLAGS, 0, &imap->mbox_stat);
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
	  mu_imap_callback (imap, MU_IMAP_CB_UIDNEXT, 0, &imap->mbox_stat);
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
	  mu_imap_callback (imap, MU_IMAP_CB_UIDVALIDITY, 0, &imap->mbox_stat);
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
	  mu_imap_callback (imap, MU_IMAP_CB_FIRST_UNSEEN, 0,
			    &imap->mbox_stat);
	}
      return;
    }

  if (mu_list_tail (resp, (void*) &arg) || arg->type != imap_eltype_string)
    arg = NULL;
  mu_imap_callback (imap, MU_IMAP_CB_OK, rcode, arg ? arg->v.string : NULL);
  if (imap->client_state == MU_IMAP_CLIENT_GREETINGS)
    {
      imap->client_state = MU_IMAP_CLIENT_READY;
      imap->session_state = MU_IMAP_SESSION_NONAUTH;
    }
}

static void
default_response (mu_imap_t imap, int code, mu_list_t resp, void *data)
{
  struct imap_list_element *arg;

  if (mu_list_tail (resp, (void*) &arg) || arg->type != imap_eltype_string)
    arg = NULL;
  mu_imap_callback (imap, code, parse_response_code (imap, resp),
		    arg ? arg->v.string : NULL);
}

static void
no_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  default_response (imap, MU_IMAP_CB_NO, resp, data);
  if (imap->client_state == MU_IMAP_CLIENT_GREETINGS)
    imap->client_state = MU_IMAP_CLIENT_ERROR;
}

static void
bad_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  default_response (imap, MU_IMAP_CB_BAD, resp, data);
  if (imap->client_state == MU_IMAP_CLIENT_GREETINGS)
    imap->client_state = MU_IMAP_CLIENT_ERROR;
}

static void
bye_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  default_response (imap, MU_IMAP_CB_BYE, resp, data);
  imap->client_state = MU_IMAP_CLIENT_CLOSING;
}

static void
preauth_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  if (imap->client_state == MU_IMAP_CLIENT_GREETINGS)
    {
      struct imap_list_element *arg;

      if (mu_list_tail (resp, (void*) &arg) || arg->type != imap_eltype_string)
	arg = NULL;
      mu_imap_callback (imap, MU_IMAP_CB_PREAUTH,
			parse_response_code (imap, resp),
			arg ? arg->v.string : NULL);
      imap->client_state = MU_IMAP_CLIENT_READY;
      imap->session_state = MU_IMAP_SESSION_AUTH;
    }
  else
    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
	      ("ignoring unexpected PREAUTH response"));
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
  int code;
};

static struct resptab resptab[] = {
  { "OK", ok_response },
  { "NO", no_response },
  { "BAD", bad_response },
  { "PREAUTH", preauth_response },
  { "BYE", bye_response },
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
	  mu_imap_callback (imap, MU_IMAP_CB_MESSAGE_COUNT, 0,
			    &imap->mbox_stat);
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
	  mu_imap_callback (imap, MU_IMAP_CB_RECENT_COUNT, 0,
			    &imap->mbox_stat);
	  return 0;
	}
    }
  else if (count == 3 &&
	   _mu_imap_list_nth_element_is_string (resp, 1, "FETCH"))
    {
      size_t msgno;

      arg = _mu_imap_list_at (resp, 0);
      if (arg && arg->type == imap_eltype_string)
	{
	  char *p;
	  msgno = strtoul (arg->v.string, &p, 10);
	  if (*p)
	    return 1;
	  
	  arg = _mu_imap_list_at (resp, 2);
	  if (arg->type == imap_eltype_list)
	    {
	      mu_list_t list;
	      
	      if (_mu_imap_parse_fetch_response (arg->v.list, &list) == 0)
		{
		  mu_imap_callback (imap, MU_IMAP_CB_FETCH, msgno, list);
		  mu_list_destroy (&list);
		}
	      return 0;
	    }
	}
    }

  return 1;
}

int
_mu_imap_process_untagged_response (mu_imap_t imap, mu_list_t list,
				    mu_imap_response_action_t fun,
				    void *data)
{
  if (_process_unsolicited_response (imap, list))
    {
      if (fun)
	fun (imap, list, data);
      else
	mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE9,
		  ("ignoring unrecognized response"));
    }
  return 0;
}

static void
default_tagged_response (mu_imap_t imap, int code, mu_list_t resp, void *data)
{
  struct imap_list_element *arg;

  if (mu_list_tail (resp, (void*) &arg) == 0 &&
      arg->type == imap_eltype_string)
    _mu_imap_seterrstrz (imap, arg->v.string);
  imap->response_code = parse_response_code (imap, resp);
  mu_imap_callback (imap, code, imap->response_code,
		    arg ? arg->v.string : NULL);
}

static void
ok_tagged_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  default_tagged_response (imap, MU_IMAP_CB_TAGGED_OK, resp, data);
}

static void
no_tagged_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  default_tagged_response (imap, MU_IMAP_CB_TAGGED_NO, resp, data);
}

static void
bad_tagged_response (mu_imap_t imap, mu_list_t resp, void *data)
{
  default_tagged_response (imap, MU_IMAP_CB_TAGGED_BAD, resp, data);
}

static struct resptab tagged_resptab[] = {
  { "OK", ok_tagged_response, MU_IMAP_OK },
  { "NO", no_tagged_response, MU_IMAP_NO },
  { "BAD", bad_tagged_response, MU_IMAP_BAD },
  { NULL }
};

static int
_std_tagged_response (mu_imap_t imap, size_t count, mu_list_t resp)
{
  struct resptab *rp;
  struct imap_list_element *arg = _mu_imap_list_at (resp, 0);

  if (!arg)
    return 1;

  if (arg->type == imap_eltype_string)
    for (rp = tagged_resptab; rp->name; rp++)
      {
	if (mu_c_strcasecmp (rp->name, arg->v.string) == 0)
	  {
	    imap->response = rp->code;
	    rp->action (imap, resp, NULL);
	    return 0;
	  }
      }
  return 1;
}

int
_mu_imap_process_tagged_response (mu_imap_t imap, mu_list_t resp)
{
  size_t count;
  
  if (mu_list_count (resp, &count))
    return 1;

  return _std_tagged_response (imap, count, resp);
}


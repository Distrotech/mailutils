/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005-2008, 2010-2012 Free Software
   Foundation, Inc.

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

#include "imap4d.h"

struct imap_auth
{
  char *name;
  imap4d_auth_handler_fp handler;
};

static mu_list_t imap_auth_list;

static int
comp (const void *item, const void *data)
{
  const struct imap_auth *p = item;
  return strcmp (p->name, (const char*) data);
}

void
auth_add (char *name, imap4d_auth_handler_fp handler)
{
  struct imap_auth *p = mu_alloc (sizeof (*p));

  p->name = name;
  p->handler = handler;
  if (!imap_auth_list)
    {
      mu_list_create (&imap_auth_list);
      mu_list_set_comparator (imap_auth_list, comp);
      mu_list_set_destroy_item (imap_auth_list, mu_list_free_item);
    }
  mu_list_append (imap_auth_list, (void*)p);
}

void
auth_remove (char *name)
{
  mu_list_remove (imap_auth_list, (void*) name);
}

static int
_auth_capa (void *item, void *usused)
{
  struct imap_auth *p = item;
  io_sendf (" AUTH=%s", p->name);
  return 0;
}

static int
_auth_try (void *item, void *data)
{
  struct imap_auth *p = item;
  struct imap4d_auth *ap = data;

  if (strcmp (p->name, ap->auth_type) == 0)
    {
      int res = p->handler (ap);
      if (res)
	return res;
    }
  return 0;
}

void
imap4d_auth_capability ()
{
  mu_list_foreach (imap_auth_list, _auth_capa, NULL);
}

/*
6.2.1.  AUTHENTICATE Command

   Arguments:  authentication mechanism name
*/

int
imap4d_authenticate (struct imap4d_session *session,
		     struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  char *auth_type;
  struct imap4d_auth adata;
  enum imap4d_auth_result res;
  
  if (imap4d_tokbuf_argc (tok) != 3)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");
  
  auth_type = imap4d_tokbuf_getarg (tok, IMAP4_ARG_1);

  if (tls_required)
    return io_completion_response (command, RESP_NO,
			           "Command disabled: Use STARTTLS first");
  
  adata.command = command;
  adata.auth_type = auth_type;
  adata.username = NULL;

  res = mu_list_foreach (imap_auth_list, _auth_try, &adata);

  switch (res)
    {
    case imap4d_auth_nosup:
      return io_completion_response (command, RESP_NO,
				     "Authentication mechanism not supported");
    case imap4d_auth_ok:
      return 0;

    case imap4d_auth_resp:
      if (adata.response == RESP_OK && adata.username)
	{
	  if (imap4d_session_setup (adata.username))
	    return io_completion_response (command, RESP_NO,
					   "User name or passwd rejected");
	  else
	    return io_completion_response (command, RESP_OK,
					   "%s authentication successful",
					   auth_type);
	}
      /* fall through */
    case imap4d_auth_fail:
      adata.response = RESP_NO;
      break;
    }
  return io_completion_response (command, adata.response,
				 "%s authentication failed", auth_type);
}


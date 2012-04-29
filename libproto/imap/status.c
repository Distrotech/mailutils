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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/assoc.h>
#include <mailutils/stream.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

#define STATUS_FLAG_MASK \
  (MU_IMAP_STAT_MESSAGE_COUNT|			\
   MU_IMAP_STAT_RECENT_COUNT|			\
   MU_IMAP_STAT_UIDNEXT|			\
   MU_IMAP_STAT_UIDVALIDITY|			\
   MU_IMAP_STAT_FIRST_UNSEEN)

struct mu_kwd _mu_imap_status_name_table[] = {
  { "MESSAGES",    MU_IMAP_STAT_MESSAGE_COUNT },
  { "RECENT",      MU_IMAP_STAT_RECENT_COUNT  },
  { "UIDNEXT",     MU_IMAP_STAT_UIDNEXT       },
  { "UIDVALIDITY", MU_IMAP_STAT_UIDVALIDITY   },
  { "UNSEEN",      MU_IMAP_STAT_FIRST_UNSEEN  },
  { NULL }
};

static int
_status_mapper (void **itmv, size_t itmc, void *call_data)
{
  struct mu_imap_stat *ps = call_data;
  struct imap_list_element *kw = itmv[0], *val = itmv[1];
  size_t value;
  char *p;
  int flag;
  
  if (kw->type != imap_eltype_string || val->type != imap_eltype_string)
    return MU_ERR_PARSE;
  if (mu_kwd_xlat_name_ci (_mu_imap_status_name_table, kw->v.string, &flag))
    return MU_ERR_PARSE;

  value = strtoul (val->v.string, &p, 10);
  if (*p)
    return MU_ERR_PARSE;

  ps->flags |= flag;

  switch (flag)
    {
    case MU_IMAP_STAT_MESSAGE_COUNT:
      ps->message_count = value;
      break;
      
    case MU_IMAP_STAT_RECENT_COUNT:
      ps->recent_count = value;
      break;
      
    case MU_IMAP_STAT_UIDNEXT:
      ps->uidnext = value;
      break;
      
    case MU_IMAP_STAT_UIDVALIDITY:
      ps->uidvalidity = value;
      break;
      
    case MU_IMAP_STAT_FIRST_UNSEEN:
      ps->first_unseen = value;
    }
  return 0;
}

struct status_data
{
  const char *mboxname;
  struct mu_imap_stat *ps;
};

static void
_status_response_action (mu_imap_t imap, mu_list_t response, void *data)
{
  struct status_data *sd = data;
  struct imap_list_element *elt;

  elt = _mu_imap_list_at (response, 0);
  if (elt && _mu_imap_list_element_is_string (elt, "STATUS"))
    {
      elt = _mu_imap_list_at (response, 1);
      if (elt && _mu_imap_list_element_is_string (elt, sd->mboxname))
	{
	  elt = _mu_imap_list_at (response, 2);
	  if (elt && elt->type == imap_eltype_list)
	    {
	      sd->ps->flags = 0;
	      mu_list_gmap (elt->v.list, _status_mapper, 2, sd->ps);
	    }
	}
    }
}

int
mu_imap_status (mu_imap_t imap, const char *mboxname, struct mu_imap_stat *ps)
{
  int status;
  int delim = 0;
  int i;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state != MU_IMAP_SESSION_AUTH &&
      imap->session_state != MU_IMAP_SESSION_SELECTED)
    return MU_ERR_SEQ;
  if (!ps)
    return MU_ERR_OUT_PTR_NULL;
  if ((ps->flags & STATUS_FLAG_MASK) == 0)
    return EINVAL;
      
  if (!mboxname)
    {
      if (imap->session_state == MU_IMAP_SESSION_SELECTED)
	{
	  if (ps)
	    *ps = imap->mbox_stat;
	  return 0;
	}
      return EINVAL;
    }
  
  if (imap->mbox_name && strcmp (imap->mbox_name, mboxname) == 0)
    {
      if (ps)
	*ps = imap->mbox_stat;
      return 0;
    }
  
  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s STATUS %s (",
				 imap->tag_str, mboxname);
      MU_IMAP_CHECK_ERROR (imap, status);
      delim = 0;
      for (i = 0; status == 0 && _mu_imap_status_name_table[i].name; i++)
	{
	  if (ps->flags & _mu_imap_status_name_table[i].tok)
	    {
	      if (delim)
		status = mu_imapio_send (imap->io, " ", 1);
	      if (status == 0)
		status = mu_imapio_printf (imap->io, "%s",
					   _mu_imap_status_name_table[i].name);
	    }
	  delim = 1;
	}
      if (status == 0)
	status = mu_imapio_send (imap->io, ")\r\n", 3);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_STATUS_RX;

    case MU_IMAP_CLIENT_STATUS_RX:
      {
	struct status_data sd = { mboxname, ps };

	status = _mu_imap_response (imap, _status_response_action, &sd);
	MU_IMAP_CHECK_EAGAIN (imap, status);
	switch (imap->response)
	  {
	  case MU_IMAP_OK:
	    break;

	  case MU_IMAP_NO:
	    status = EACCES;
	    break;

	  case MU_IMAP_BAD:
	    status = MU_ERR_BADREPLY;
	    break;
	  }
	imap->client_state = MU_IMAP_CLIENT_READY;
      }
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

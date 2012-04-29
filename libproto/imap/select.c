/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/imaputil.h>
#include <mailutils/sys/imap.h>

static int
_collect_flags (void *item, void *data)
{
  struct imap_list_element *elt = item;
  int *args = data;

  if (elt->type == imap_eltype_string)
    mu_imap_flag_to_attribute (elt->v.string, args);
  return 0;
}

int
_mu_imap_collect_flags (struct imap_list_element *arg, int *res)
{
  if (arg->type != imap_eltype_list)
    return EINVAL;
  *res = 0;
  mu_list_foreach (arg->v.list, _collect_flags, res);
  return 0;
}

static void
_select_response_action (mu_imap_t imap, mu_list_t response, void *data)
{
  struct imap_list_element *elt;
  
  elt = _mu_imap_list_at (response, 0);
  if (elt && _mu_imap_list_element_is_string (elt, "FLAGS"))
    {
      struct imap_list_element *arg = _mu_imap_list_at (response, 1);
      if (arg &&
	  _mu_imap_collect_flags (arg, &imap->mbox_stat.defined_flags) == 0)
	imap->mbox_stat.flags |= MU_IMAP_STAT_DEFINED_FLAGS;
    }
}

int
mu_imap_select (mu_imap_t imap, const char *mbox, int writable,
		struct mu_imap_stat *ps)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state != MU_IMAP_SESSION_AUTH &&
      imap->session_state != MU_IMAP_SESSION_SELECTED)
    return MU_ERR_SEQ;

  if (!mbox)
    {
      if (imap->session_state == MU_IMAP_SESSION_SELECTED)
	{
	  if (ps)
	    *ps = imap->mbox_stat;
	  return 0;
	}
      return MU_ERR_INFO_UNAVAILABLE;
    }
  
  if (imap->mbox_name && strcmp (imap->mbox_name, mbox) == 0
      && writable == imap->mbox_writable)
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
      status = mu_imapio_send_command (imap->io, imap->tag_str, NULL,
				       writable ? "SELECT" : "EXAMINE",
				       mbox, NULL);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_SELECT_RX;

    case MU_IMAP_CLIENT_SELECT_RX:
      memset (&imap->mbox_stat, 0, sizeof (imap->mbox_stat));
      status = _mu_imap_response (imap, _select_response_action, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->response)
	{
	case MU_IMAP_OK:
	  imap->session_state = MU_IMAP_SESSION_SELECTED;
	  free (imap->mbox_name);
	  imap->mbox_name = strdup (mbox);
	  if (!imap->mbox_name)
	    {
	      imap->client_state = MU_IMAP_CLIENT_ERROR;
	      return errno;
	    }
	  imap->mbox_writable = writable;
	  if (ps)
	    *ps = imap->mbox_stat;
	  break;

	case MU_IMAP_NO:
	  status = EACCES;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  break;
	}
      imap->client_state = MU_IMAP_CLIENT_READY;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

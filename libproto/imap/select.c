/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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
  char *p;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;
  if (imap->imap_state != MU_IMAP_STATE_AUTH &&
      imap->imap_state != MU_IMAP_STATE_SELECTED)
    return MU_ERR_SEQ;

  if (!mbox)
    {
      if (imap->imap_state == MU_IMAP_STATE_SELECTED)
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
  
  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s %s %s\r\n",
				 imap->tag_str,
				 writable ? "SELECT" : "EXAMINE",
				 mbox);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->state = MU_IMAP_SELECT_RX;

    case MU_IMAP_SELECT_RX:
      status = _mu_imap_response (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->resp_code)
	{
	case MU_IMAP_OK:
	  imap->imap_state = MU_IMAP_STATE_SELECTED;
	  free (imap->mbox_name);
	  imap->mbox_name = strdup (mbox);
	  if (!imap->mbox_name)
	    {
	      imap->state = MU_IMAP_ERROR;
	      return errno;
	    }
	  imap->mbox_writable = writable;
	  memset (&imap->mbox_stat, 0, sizeof (imap->mbox_stat));
	  mu_imap_foreach_response (imap, _select_response_action, NULL);
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
      imap->state = MU_IMAP_CONNECTED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

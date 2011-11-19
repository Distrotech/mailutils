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

static int
_parse_stat (void *item, void *data)
{
  struct imap_list_element *response = item;
  mu_imap_t imap = data;
  struct imap_list_element *elt;
  size_t count;
  int rc;
  char *p;
  
  if (response->type != imap_eltype_list)
    return 0;

  mu_list_count (response->v.list, &count);
  
  rc = mu_list_get (response->v.list, 0, (void*) &elt);
  if (rc)
    return rc;
  
  if (_mu_imap_list_element_is_string (elt, "OK"))
    {
      struct imap_list_element *arg;
      
      if (count < 3)
	return 0; /* ignore the line */
      rc = mu_list_get (response->v.list, 1, (void*) &elt);
      if (rc)
	return rc;
      rc = mu_list_get (response->v.list, 2, (void*) &arg);
      if (rc)
	return rc;
      
      if (_mu_imap_list_element_is_string (elt, "[UIDVALIDITY"))
	{
	  if (arg->type != imap_eltype_string)
	    return 0;
	  imap->mbox_stat.uidvalidity = strtoul (arg->v.string, &p, 10);
	  if (*p == ']')
	    imap->mbox_stat.flags |= MU_IMAP_STAT_UIDVALIDITY;
	}
      else if (_mu_imap_list_element_is_string (elt, "[UIDNEXT"))
	{
	  if (arg->type != imap_eltype_string)
	    return 0;
	  imap->mbox_stat.uidnext = strtoul (arg->v.string, &p, 10);
	  if (*p == ']')
	    imap->mbox_stat.flags |= MU_IMAP_STAT_UIDNEXT;
	}
      else if (_mu_imap_list_element_is_string (elt, "[UNSEEN"))
	{
	  if (arg->type != imap_eltype_string)
	    return 0;
	  imap->mbox_stat.first_unseen = strtoul (arg->v.string, &p, 10);
	  if (*p == ']')
	    imap->mbox_stat.flags |= MU_IMAP_STAT_FIRST_UNSEEN;
	}
      else if (_mu_imap_list_element_is_string (elt, "[PERMANENTFLAGS"))
	{
	  if (arg->type != imap_eltype_list)
	    return 0;
	  mu_list_foreach (arg->v.list, _collect_flags,
		      &imap->mbox_stat.permanent_flags);
	  imap->mbox_stat.flags |= MU_IMAP_STAT_PERMANENT_FLAGS;
	}
    }
  else if (_mu_imap_list_element_is_string (elt, "FLAGS"))
    {
      struct imap_list_element *arg;
      rc = mu_list_get (response->v.list, 1, (void*) &arg);
      if (rc)
	return 0;
      if (arg->type != imap_eltype_list)
	return 0;
      mu_list_foreach (arg->v.list, _collect_flags, &imap->mbox_stat.defined_flags);
      imap->mbox_stat.flags |= MU_IMAP_STAT_DEFINED_FLAGS;
    }
  else if (count == 2)
    {
      struct imap_list_element *arg;
      rc = mu_list_get (response->v.list, 1, (void*) &arg);
      if (rc)
	return rc;
      if (_mu_imap_list_element_is_string (arg, "EXISTS"))
	{
	  imap->mbox_stat.message_count = strtoul (elt->v.string, &p, 10);
	  if (*p == 0)
	    imap->mbox_stat.flags |= MU_IMAP_STAT_MESSAGE_COUNT;
	}
      else if (_mu_imap_list_element_is_string (arg, "RECENT"))
	{
	  imap->mbox_stat.recent_count = strtoul (elt->v.string, &p, 10);
	  if (*p == 0)
	    imap->mbox_stat.flags |= MU_IMAP_STAT_RECENT_COUNT;
	}
    }
  
  return 0;
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
	  mu_list_foreach (imap->untagged_resp, _parse_stat, imap);
	  if (ps)
	    *ps = imap->mbox_stat;
	  break;

	case MU_IMAP_NO:
	  status = EACCES;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  if (mu_imapio_reply_string (imap->io, 2, &p) == 0)
	    {
	      _mu_imap_seterrstr (imap, p, strlen (p));
	      free (p);
	    }
	  break;
	}
      imap->state = MU_IMAP_CONNECTED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

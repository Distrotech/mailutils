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

void
_id_free (void *data)
{
  char *s = *(char**)data;
  free (s);
}

static int
_id_mapper (void **itmv, size_t itmc, void *call_data)
{
  int rc;
  mu_assoc_t assoc = call_data;
  struct imap_list_element *key = itmv[0], *val = itmv[1];
  if (key->type != imap_eltype_string || val->type != imap_eltype_string)
    return MU_ERR_FAILURE;
  rc = mu_assoc_install (assoc, key->v.string, &val->v.string);
  if (rc == 0)
    val->v.string = NULL;
  return rc;
}

static void
_id_response_action (mu_imap_t imap, mu_list_t response, void *data)
{
  mu_assoc_t assoc = data;
  struct imap_list_element *elt;

  elt = _mu_imap_list_at (response, 0);
  if (elt && _mu_imap_list_element_is_string (elt, "ID"))
    {
      elt = _mu_imap_list_at (response, 1);
      if (elt->type == imap_eltype_list)
	mu_list_gmap (elt->v.list, _id_mapper, 2, assoc);
    }
}

static int
parse_id_reply (mu_imap_t imap, mu_assoc_t *passoc)
{
  int rc;
  mu_assoc_t assoc;
  
  rc = mu_assoc_create (&assoc, sizeof (char**), MU_ASSOC_ICASE);
  if (rc)
    return rc;
  mu_assoc_set_free (assoc, _id_free);
  
  rc = mu_imap_foreach_response (imap, _id_response_action, assoc);
  if (rc)
    return rc;
  *passoc = assoc;
  return 0;
}
  
int
mu_imap_id (mu_imap_t imap, char **idenv, mu_assoc_t *passoc)
{
  int status;
  char *p;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;

  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s ID ", imap->tag_str);
      MU_IMAP_CHECK_ERROR (imap, status);
      if (!idenv)
	status = mu_imapio_printf (imap->io, "NIL");
      else
	{
	  if (idenv[0])
	    {
	      int i;
	      char *delim = "(";
	      for (i = 0; idenv[i]; i++)
		{
		  status = mu_imapio_printf (imap->io, "%s\"%s\"",
					     delim, idenv[i]);
		  MU_IMAP_CHECK_ERROR (imap, status);
		  
		  delim = " ";
		  if (status)
		    break;
		}
	      status = mu_imapio_printf (imap->io, ")");
	    }
	  else
	    status = mu_imapio_printf (imap->io, "()");
	}
      MU_IMAP_CHECK_ERROR (imap, status);
      status = mu_imapio_printf (imap->io, "\r\n");
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->state = MU_IMAP_ID_RX;

    case MU_IMAP_ID_RX:
      status = _mu_imap_response (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->resp_code)
	{
	case MU_IMAP_OK:
	  imap->imap_state = MU_IMAP_STATE_AUTH;
	  if (passoc)
	    status = parse_id_reply (imap, passoc);
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
      

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

struct id_convert_state
{
  int item;
  mu_assoc_t assoc;
  int ret;
};

static int
_id_convert (void *item, void *data)
{
  struct imap_list_element *elt = item;
  struct id_convert_state *stp = data;

  switch (stp->item)
    {
    case 0:
      if (!(elt->type == imap_eltype_string &&
	    strcmp (elt->v.string, "ID") == 0))
	{
	  stp->ret = MU_ERR_PARSE;
	  return 1;
	}
      stp->item++;
      return 0;

    case 1:
      if (elt->type == imap_eltype_list)
	{
	  mu_iterator_t itr;

	  mu_list_get_iterator (elt->v.list, &itr);
	  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	       mu_iterator_next (itr))
	    {
	      char *key, *val;
	      mu_iterator_current (itr, (void **) &elt);

	      if (elt->type != imap_eltype_string)
		break;
	      key = elt->v.string;
	      elt->v.string = NULL;

	      mu_iterator_next (itr);
	      if (mu_iterator_is_done (itr))
		break;
	      mu_iterator_current (itr, (void **) &elt);
	      if (elt->type != imap_eltype_string)
		break;
	      val = elt->v.string;
	      elt->v.string = NULL;
	      mu_assoc_install (stp->assoc, key, &val);
	    }
	}
    }
  return 1;
}	

static int
parse_id_reply (mu_imap_t imap, mu_assoc_t *passoc)
{
  int rc;
  struct imap_list_element const *response;
  struct id_convert_state st;
  mu_assoc_t assoc;
  
  rc = mu_assoc_create (&assoc, sizeof (char**), MU_ASSOC_ICASE);
  if (rc)
    return rc;
  mu_assoc_set_free (assoc, _id_free);
  
  rc = mu_list_get (imap->untagged_resp, 0, (void*) &response);
  *passoc = assoc;
  if (rc == MU_ERR_NOENT)
    return 0;

  st.item = 0;
  st.assoc = assoc;
  st.ret = 0;
  mu_list_do (response->v.list, _id_convert, &st);
  return st.ret;
}
  
  
int
mu_imap_id (mu_imap_t imap, char **idenv, mu_assoc_t *passoc)
{
  int status;
  
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
	  break;
	}
      imap->state = MU_IMAP_CONNECTED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}
      

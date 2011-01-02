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
#include <mailutils/cstr.h>
#include <mailutils/wordsplit.h>
#include <mailutils/stream.h>
#include <mailutils/sys/imap.h>

static int
id_comp (const void *item, const void *value)
{
  const char *id = item;
  const char *needle = value;
  return mu_c_strcasecmp (id, needle);
}

static int
parse_id_reply (mu_imap_t imap, mu_list_t *plist)
{
  mu_list_t list;
  int rc;
  const char *response;
  struct mu_wordsplit ws;
  size_t i;
    
  rc = mu_list_create (&list);
  if (rc)
    return rc;
  mu_list_set_comparator (list, id_comp);
  mu_list_set_destroy_item (list, mu_list_free_item);
  
  rc = mu_list_get (imap->untagged_resp, 0, (void*) &response);
  if (rc == MU_ERR_NOENT)
    {
      *plist = list;
      return 0;
    }
  else if (rc)
    {
      mu_list_destroy (&list);
      return rc;
    }

  ws.ws_delim = "() \t";
  if (mu_wordsplit (response, &ws,
		    MU_WRDSF_NOVAR | MU_WRDSF_NOCMD |
		    MU_WRDSF_QUOTE | MU_WRDSF_DELIM |
		    MU_WRDSF_SQUEEZE_DELIMS |
		    MU_WRDSF_WS))
    {
      int ec = errno;
      mu_error ("mu_imap_id: cannot split line: %s",
		mu_wordsplit_strerror (&ws));
      mu_list_destroy (&list);
      return ec;
    }

  for (i = 1; i < ws.ws_wordc; i += 2)
    {
      size_t len, l1, l2;
      char *elt;
      
      if (i + 1 == ws.ws_wordc)
	break;
      l1 = strlen (ws.ws_wordv[i]);
      l2 = strlen (ws.ws_wordv[i+1]);
      len = l1 + l2 + 1;
      elt = malloc (len + 1);
      if (!elt)
	break;

      memcpy (elt, ws.ws_wordv[i], l1);
      elt[l1] = 0;
      memcpy (elt + l1 + 1, ws.ws_wordv[i+1], l2);
      elt[len] = 0;
      mu_list_append (list, elt);
    }
  mu_wordsplit_free (&ws);
  *plist = list;
  return 0;
}

int
mu_imap_id (mu_imap_t imap, char **idenv, mu_list_t *plist)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->carrier)
    return MU_ERR_NO_TRANSPORT;
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;

  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_stream_printf (imap->carrier, "%s ID ",
				 imap->tag_str);
      MU_IMAP_CHECK_ERROR (imap, status);
      if (!idenv)
	status = mu_stream_printf (imap->carrier, "NIL");
      else
	{
	  if (idenv[0])
	    {
	      int i;
	      char *delim = "(";
	      for (i = 0; idenv[i]; i++)
		{
		  status = mu_stream_printf (imap->carrier, "%s\"%s\"",
					     delim, idenv[i]);
		  MU_IMAP_CHECK_ERROR (imap, status);
		  
		  delim = " ";
		  if (status)
		    break;
		}
	      status = mu_stream_printf (imap->carrier, ")");
	    }
	  else
	    status = mu_stream_printf (imap->carrier, "()");
	}
      MU_IMAP_CHECK_ERROR (imap, status);
      status = mu_stream_printf (imap->carrier, "\r\n");
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
	  if (plist)
	    status = parse_id_reply (imap, plist);
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
      

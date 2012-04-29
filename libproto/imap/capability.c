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

#include <string.h>
#include <mailutils/types.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/sys/imap.h>

static int
capa_comp (const void *item, const void *value)
{
  const char *capa = item;
  const char *needle = value;
  for (; *needle; capa++, needle++)
    {
      if (!*capa)
	return 1;
      if (mu_tolower (*capa) != mu_tolower (*needle))
	return 1;
    }
  return !(*capa == 0 || *capa == '=');
}

static int
_map_capa (void **itmv, size_t itmc, void *call_data)
{
  int *n = call_data;
  struct imap_list_element *elt = itmv[0];

  if (elt->type != imap_eltype_string)
    return MU_LIST_MAP_STOP;
  if (*n == 0)
    {
      *n = 1;
      return MU_LIST_MAP_SKIP;
    }
  itmv[0] = elt->v.string;
  elt->v.string = NULL;
  return MU_LIST_MAP_OK;
}

static void
_capability_response_action (mu_imap_t imap, mu_list_t response, void *data)
{
  struct imap_list_element *elt = _mu_imap_list_at (response, 0);
  if (elt && _mu_imap_list_element_is_string (elt, "CAPABILITY"))
    {
      int n = 0;
      mu_list_map (response, _map_capa, &n, 1, &imap->capa);
    }
}

int
mu_imap_capability (mu_imap_t imap, int reread, mu_iterator_t *piter)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state == MU_IMAP_SESSION_INIT ||
      imap->client_state != MU_IMAP_CLIENT_READY)
    return MU_ERR_SEQ;

  if (imap->capa)
    {
      if (!reread)
	{
	  if (!piter)
	    return 0;
	  return mu_list_get_iterator (imap->capa, piter);
	}
      mu_list_destroy (&imap->capa);
    }

  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_send_command (imap->io, imap->tag_str, NULL,
				       "CAPABILITY", NULL); 
      MU_IMAP_CHECK_EAGAIN (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_CAPABILITY_RX;

    case MU_IMAP_CLIENT_CAPABILITY_RX:
      status = _mu_imap_response (imap, _capability_response_action,
				  NULL);
      imap->client_state = MU_IMAP_CLIENT_READY;
      MU_IMAP_CHECK_EAGAIN (imap, status);
      if (imap->response != MU_IMAP_OK)
	return MU_ERR_REPLY;
      else
	{
	  mu_list_set_comparator (imap->capa, capa_comp);
	  mu_list_set_destroy_item (imap->capa, mu_list_free_item);

	  if (piter)
	    status = mu_list_get_iterator (imap->capa, piter);
	  else
	    status = 0;
	}  
      break;
      
    case MU_IMAP_CLIENT_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}
    

  

  
  


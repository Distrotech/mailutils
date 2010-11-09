/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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

#include <mailutils/types.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/list.h>
#include <mailutils/wordsplit.h>
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

int
mu_imap_capability (mu_imap_t imap, int reread, mu_iterator_t *piter)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->carrier)
    return MU_ERR_NO_TRANSPORT;
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;

  if (imap->capa)
    {
      if (!reread)
	{
	  if (!piter)
	    return 0;
	  return mu_list_get_iterator (imap->capa, piter);
	}
      mu_list_clear (imap->capa);
    }
  else
    {
      status = mu_list_create (&imap->capa);
      MU_IMAP_CHECK_ERROR (imap, status);
      mu_list_set_comparator (imap->capa, capa_comp);
      mu_list_set_destroy_item (imap->capa, mu_list_free_item);
    }

  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_stream_printf (imap->carrier, "%s CAPABILITY\r\n",
				 imap->tag_str); 
      MU_IMAP_CHECK_EAGAIN (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->state = MU_IMAP_CAPABILITY_RX;

    case MU_IMAP_CAPABILITY_RX:
      status = _mu_imap_response (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      if (imap->resp_code != MU_IMAP_OK)
	return MU_ERR_REPLY;
      else
	{
	  size_t count;
	  char *str;
	  
	  imap->state = MU_IMAP_CONNECTED;
	  mu_list_count (imap->untagged_resp, &count);
	  if (mu_list_get (imap->untagged_resp, 0, (void*)&str) == 0)
	    {
	      size_t i;
	      
	      struct mu_wordsplit ws;

	      if (mu_wordsplit (str, &ws,
				MU_WRDSF_NOVAR | MU_WRDSF_NOCMD |
				MU_WRDSF_QUOTE | MU_WRDSF_SQUEEZE_DELIMS))
		{
		  int ec = errno;
		  mu_error ("mu_imap_capability: cannot split line: %s",
			    mu_wordsplit_strerror (&ws));
		  return ec;
		}
	      if (ws.ws_wordc > 1 &&
		  mu_c_strcasecmp (ws.ws_wordv[0], "CAPABILITY") == 0)
		{
		  for (i = 1; i < ws.ws_wordc; i++)
		    {
		      mu_list_append (imap->capa, ws.ws_wordv[i]);
		      ws.ws_wordv[i] = NULL;
		    }
		}
	      mu_wordsplit_free (&ws);
	    }
	  if (piter)
	    status = mu_list_get_iterator (imap->capa, piter);
	  else
	    status = 0;
	}  
      break;
      
    case MU_IMAP_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}
    

  

  
  


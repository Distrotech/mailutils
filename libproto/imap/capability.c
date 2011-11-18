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

int
mu_imap_capability (mu_imap_t imap, int reread, mu_iterator_t *piter)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
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
      status = mu_imapio_printf (imap->io, "%s CAPABILITY\r\n",
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
	  struct imap_list_element *elt;
	  
	  imap->state = MU_IMAP_CONNECTED;
	  mu_list_count (imap->untagged_resp, &count);
	  if (mu_list_get (imap->untagged_resp, 0, (void*)&elt) == 0)
	    {
	      /* Top-level elements are always of imap_eltype_list type. */
	      mu_iterator_t itr;
	      
	      mu_list_get_iterator (elt->v.list, &itr);
	      mu_iterator_first (itr);
	      if (mu_iterator_is_done (itr))
		return MU_ERR_PARSE;
	      mu_iterator_current (itr, (void **) &elt);
	      if (elt->type == imap_eltype_string &&
		  strcmp (elt->v.string, "CAPABILITY") == 0)
		{
		  for (mu_iterator_next (itr); !mu_iterator_is_done (itr);
		       mu_iterator_next (itr))
		    {
		      mu_iterator_current (itr, (void **) &elt);
		      if (elt->type == imap_eltype_string)
			{
			  mu_list_append (imap->capa, elt->v.string);
			  elt->v.string = NULL;
			}
		    }
		}
	      mu_iterator_destroy (&itr);
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
    

  

  
  


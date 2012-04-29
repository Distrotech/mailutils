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
#include <errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>

int
_mu_imap_response (mu_imap_t imap, mu_imap_response_action_t fun,
		   void *data)
{
  int status = 0;

  if (imap == NULL)
    return EINVAL;

  if (MU_IMAP_FISSET (imap, MU_IMAP_RESP))
    return 0;

  _mu_imap_clrerrstr (imap);
  
  while (1)
    {
      status = mu_imapio_getline (imap->io);
      if (status == 0)
	{
	  char **wv;
	  size_t wc;
	  
	  mu_imapio_get_words (imap->io, &wc, &wv);
	  if (wc == 0)
	    {
	      imap->client_state = MU_IMAP_CLIENT_ERROR;
	      status = MU_ERR_BADREPLY;/* FIXME: ECONNRESET ? */
	      break;
	    }
	    
	  if (strcmp (wv[0], "*") == 0)
	    {
	      mu_list_t list;
	      status = _mu_imap_untagged_response_to_list (imap, &list);
	      if (status)
		break;
	      _mu_imap_process_untagged_response (imap, list, fun, data);
	      mu_list_destroy (&list);
	      continue;
	    }
	  else if (strlen (wv[0]) == imap->tag_len &&
		   memcmp (wv[0], imap->tag_str, imap->tag_len) == 0)
	    {
	      /* Handle the tagged response */
	      if (wc < 2)
		{
		  /*imap->client_state = MU_IMAP_CLIENT_ERROR;*/
		  status = MU_ERR_BADREPLY;
		}
	      else
		{
		  mu_list_t list;
		  status = _mu_imap_untagged_response_to_list (imap, &list);
		  if (status == 0)
		    {
		      if (_mu_imap_process_tagged_response (imap, list))
			status = MU_ERR_BADREPLY;
		      mu_list_destroy (&list);
		    }
		}
	      MU_IMAP_FSET (imap, MU_IMAP_RESP);
	    }
	  else
	    {
	      imap->client_state = MU_IMAP_CLIENT_ERROR;
	      status = MU_ERR_BADREPLY;
	    }
	}
      else
	imap->client_state = MU_IMAP_CLIENT_ERROR;
      break;
    }
  return status;
}

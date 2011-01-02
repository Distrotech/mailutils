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
#include <errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>

#define IS_PREFIX(s, len, pfx)				\
  ((len) >= sizeof (pfx) - 1 &&				\
   memcmp ((s), (pfx), sizeof (pfx) - 1) == 0 &&	\
   s[sizeof (pfx) - 1] == ' ')

int
_mu_imap_response (mu_imap_t imap)
{
  size_t n = 0;
  int status = 0;

  if (imap == NULL)
    return EINVAL;

  if (MU_IMAP_FISSET (imap, MU_IMAP_RESP))
    return 0;

  _mu_imap_clrerrstr (imap);
  if (imap->untagged_resp)
    mu_list_clear (imap->untagged_resp);
  else
    {
      status = mu_list_create (&imap->untagged_resp);
      MU_IMAP_CHECK_ERROR (imap, status);
      mu_list_set_destroy_item (imap->untagged_resp, mu_list_free_item);
    }

  while (1)
    {
      status = mu_stream_getline (imap->carrier, &imap->rdbuf,
				  &imap->rdsize, NULL);
      if (status == 0)
	{
	  n = mu_rtrim_class (imap->rdbuf, MU_CTYPE_SPACE);
	  if (imap->rdbuf[0] == '*' && imap->rdbuf[1] == ' ')
	    {
	      char *p = mu_str_skip_cset (imap->rdbuf + 2, " ");
	      mu_list_append (imap->untagged_resp, strdup (p));
	    }
	  else if (n > imap->tag_len + 3 &&
		   memcmp (imap->rdbuf, imap->tag_str, imap->tag_len) == 0
		   && imap->rdbuf[imap->tag_len] == ' ')
	    {
	      char *p = mu_str_skip_cset (imap->rdbuf + imap->tag_len, " ");
	      size_t len = strlen (p);

	      if (len >= imap->tagsize)
		{
		  char *np = realloc (imap->tagbuf, len + 1);
		  if (!np)
		    {
		      imap->state = MU_IMAP_ERROR;
		      return ENOMEM;
		    }
		  imap->tagsize = len + 1;
		  imap->tagbuf = np;
		}
	      strcpy (imap->tagbuf, p);
	      if (IS_PREFIX (p, len, "OK"))
		{
		  imap->resp_code = MU_IMAP_OK;
		  p = mu_str_skip_cset (p + 2, " ");
		  _mu_imap_seterrstr (imap, p, strlen (p));
		}
	      else if (IS_PREFIX (p, len, "NO"))
		{
		  imap->resp_code = MU_IMAP_NO;
		  p = mu_str_skip_cset (p + 2, " ");
		  _mu_imap_seterrstr (imap, p, strlen (p));
		}
	      else if (IS_PREFIX (p, len, "BAD"))
		{
		  imap->resp_code = MU_IMAP_BAD;
		  p = mu_str_skip_cset (p + 2, " ");
		  _mu_imap_seterrstr (imap, p, strlen (p));
		}
	      else
		status = MU_ERR_BADREPLY;
	      MU_IMAP_FSET (imap, MU_IMAP_RESP);
	      break;
	    }
	  else
	    {
	      imap->state = MU_IMAP_ERROR;
	      return MU_ERR_BADREPLY;
	    }
	}
      else
	{
	  imap->state = MU_IMAP_ERROR;
	  return status;
	}
    }
  return status;
}
	  

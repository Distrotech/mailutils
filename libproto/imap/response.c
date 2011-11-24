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

static void
response_to_errstr (mu_imap_t imap, size_t argc, char **argv)
{
  if (argc && strcmp (argv[argc-1], "]"))
    _mu_imap_seterrstrz (imap, argv[argc-1]);
}

int
_mu_imap_response (mu_imap_t imap)
{
  int status = 0;

  if (imap == NULL)
    return EINVAL;

  if (MU_IMAP_FISSET (imap, MU_IMAP_RESP))
    return 0;

  _mu_imap_clrerrstr (imap);
  status = _mu_imap_untagged_response_clear (imap);
  if (status)
    return status;
  
  while (1)
    {
      status = mu_imapio_getline (imap->io);
      if (status == 0)
	{
	  char **wv;
	  size_t wc;
	  
	  mu_imapio_get_words (imap->io, &wc, &wv);
	  if (strcmp (wv[0], "*") == 0)
	    {
	      _mu_imap_untagged_response_add (imap);/* FIXME: error checking */
	      continue;
	    }
	  else if (strlen (wv[0]) == imap->tag_len &&
		   memcmp (wv[0], imap->tag_str, imap->tag_len) == 0)
	    {
	      /* Handle the tagged response */
	      if (wc < 2)
		{
		  /*imap->state = MU_IMAP_ERROR;*/
		  status = MU_ERR_BADREPLY;
		}
	      else if (strcmp (wv[1], "OK") == 0)
		{
		  imap->resp_code = MU_IMAP_OK;
		  response_to_errstr (imap, wc, wv);
		}
	      else if (strcmp (wv[1], "NO") == 0)
		{
		  imap->resp_code = MU_IMAP_NO;
		  response_to_errstr (imap, wc, wv);
		}
	      else if (strcmp (wv[1], "BAD") == 0)
		{
		  imap->resp_code = MU_IMAP_BAD;
		  response_to_errstr (imap, wc, wv);
		}
	      else
		status = MU_ERR_BADREPLY;
	      MU_IMAP_FSET (imap, MU_IMAP_RESP);
	    }
	  else
	    {
	      imap->state = MU_IMAP_ERROR;
	      status = MU_ERR_BADREPLY;
	    }
	}
      else
	imap->state = MU_IMAP_ERROR;
      break;
    }
  return status;
}

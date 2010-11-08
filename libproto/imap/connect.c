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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/errno.h>
#include <mailutils/wordsplit.h>
#include <mailutils/sys/imap.h>


int
mu_imap_connect (mu_imap_t imap)
{
  int status;

  if (imap == NULL)
    return EINVAL;
  if (imap->carrier == NULL)
    return EINVAL;
  switch (imap->state)
    {
    default:
    case MU_IMAP_NO_STATE:
      status = mu_imap_disconnect (imap);
      if (status != 0)
	{
	  /* Sleep for 2 seconds (FIXME: Must be configurable) */
	  struct timeval tval;
	  tval.tv_sec = 2;
	  tval.tv_usec = 0;
	  select (0, NULL, NULL, NULL, &tval);
	}
      imap->state = MU_IMAP_CONNECT;

    case MU_IMAP_CONNECT:
      /* Establish the connection.  */
      if (!mu_stream_is_open (imap->carrier))
        {
          status = mu_stream_open (imap->carrier);
          MU_IMAP_CHECK_EAGAIN (imap, status);
          MU_IMAP_FCLR (imap, MU_IMAP_RESP);
        }
      imap->state = MU_IMAP_GREETINGS;

    case MU_IMAP_GREETINGS:
      status = mu_stream_getline (imap->carrier, &imap->rdbuf,
				  &imap->rdsize, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      if (imap->rdsize < 2 ||
	  !(imap->rdbuf[0] == '*' && imap->rdbuf[1] == ' '))
	{
	  mu_error ("mu_imap_connect: invalid server response: %s",
		    imap->rdbuf);
	  imap->state = MU_IMAP_ERROR;
	  return MU_ERR_BADREPLY;
	}
      else
	{
	  struct mu_wordsplit ws;
	  
 	  if (mu_wordsplit (imap->rdbuf, &ws,
			    MU_WRDSF_NOVAR | MU_WRDSF_NOCMD |
			    MU_WRDSF_SQUEEZE_DELIMS))
	    {
	      int ec = errno;
	      mu_error ("mu_imap_connect: cannot split line: %s",
			mu_wordsplit_strerror (&ws));
	      imap->state = MU_IMAP_ERROR;
	      return ec;
	    }
	  if (ws.ws_wordc < 2)
	    status = MU_ERR_BADREPLY;
	  else if (strcmp (ws.ws_wordv[1], "BYE") == 0)
	    {
	      status = EACCES;
	      _mu_imap_seterrstr (imap, imap->rdbuf + 2,
				  strlen (imap->rdbuf + 2));
	    }
	  else if (strcmp (ws.ws_wordv[1], "PREAUTH") == 0)
	    {
	      status = 0;
	      imap->state = MU_IMAP_CONNECTED;
	      imap->imap_state = MU_IMAP_STATE_AUTH;
	    }
	  else if (strcmp (ws.ws_wordv[1], "OK") == 0)
	    {
	      status = 0;
	      imap->state = MU_IMAP_CONNECTED;
	      imap->imap_state = MU_IMAP_STATE_NONAUTH;
	    }
	  else
	    {
	      status = MU_ERR_BADREPLY;
	      imap->state = MU_IMAP_ERROR;
	    }
	  mu_wordsplit_free (&ws);
	}
    }
  
  return status;
}

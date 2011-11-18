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
#include <sys/select.h>
#include <sys/time.h>
#include <mailutils/errno.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imap.h>
#include <mailutils/sys/imapio.h>


int
mu_imap_connect (mu_imap_t imap)
{
  int status;
  size_t wc;
  char **wv;
  char *bufptr;
  size_t bufsize;
  
  if (imap == NULL)
    return EINVAL;
  if (imap->io == NULL)
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
      if (!mu_stream_is_open (imap->io->_imap_stream))
        {
          status = mu_stream_open (imap->io->_imap_stream);
          MU_IMAP_CHECK_EAGAIN (imap, status);
          MU_IMAP_FCLR (imap, MU_IMAP_RESP);
        }
      imap->state = MU_IMAP_GREETINGS;

    case MU_IMAP_GREETINGS:
      status = mu_imapio_getline (imap->io);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      mu_imapio_get_words (imap->io, &wc, &wv);
      if (wc < 2 || strcmp (wv[0], "*"))
	{
	  mu_imapio_getbuf (imap->io, &bufptr, &bufsize);
	  mu_error ("mu_imap_connect: invalid server response: %s",
		    bufptr);
	  imap->state = MU_IMAP_ERROR;
	  return MU_ERR_BADREPLY;
	}
      else if (strcmp (wv[1], "BYE") == 0)
	{
	  status = EACCES;
	  mu_imapio_getbuf (imap->io, &bufptr, &bufsize);
	  _mu_imap_seterrstr (imap, bufptr + 2, bufsize - 2);
	}
      else if (strcmp (wv[1], "PREAUTH") == 0)
	{
	  status = 0;
	  imap->state = MU_IMAP_CONNECTED;
	  imap->imap_state = MU_IMAP_STATE_AUTH;
	}
      else if (strcmp (wv[1], "OK") == 0)
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
    }
  
  return status;
}

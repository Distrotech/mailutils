/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004, 2007, 2010-2012 Free Software
   Foundation, Inc.

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
#include <stdlib.h>
#include <errno.h>
#include <mailutils/sys/nntp.h>

int
mu_nntp_newnews (mu_nntp_t nntp, const char *wildmat, unsigned int year, unsigned int month, unsigned int day,
		 unsigned int hour, unsigned int minute, unsigned int second, int is_gmt, mu_iterator_t *piterator)
{
  int status = 0;

  if (nntp == NULL)
    return EINVAL;

  switch (nntp->state)
    {
    case MU_NNTP_NO_STATE:
      if (wildmat == NULL || *wildmat == '\0')
	{
	  if (is_gmt > 0)
	    status = mu_nntp_writeline (nntp, "NEWNEWS %.4d%.2d%.2d %.2d%.2d%.2d GMT\r\n", year, month, day, hour, minute, second);
	  else
	    status = mu_nntp_writeline (nntp, "NEWNEWS %.4d%.2d%.2d %.2d%.2d%.2d\r\n", year, month, day, hour, minute, second);
	}
      else
	{
	  if (is_gmt > 0)
	    {
	      status = mu_nntp_writeline (nntp, "NEWNEWS %s %.4d%.2d%.2d %.2d%.2d%.2d GMT\r\n", wildmat, year, month, day,
					  hour, minute, second);
	    }
	  else
	    {
	      status = mu_nntp_writeline (nntp, "NEWNEWS %s %.4d%.2d%.2d %.2d%.2d%.2d\r\n", wildmat, year, month, day,
					  hour, minute, second);
	    }
	}
      MU_NNTP_CHECK_ERROR (nntp, status);
      mu_nntp_debug_cmd (nntp);
      nntp->state = MU_NNTP_NEWNEWS;

    case MU_NNTP_NEWNEWS:
      status = mu_nntp_send (nntp);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->acknowledge = 0;
      nntp->state = MU_NNTP_NEWNEWS_ACK;

    case MU_NNTP_NEWNEWS_ACK:
      status = mu_nntp_response (nntp, NULL, 0, NULL);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      mu_nntp_debug_ack (nntp);
      MU_NNTP_CHECK_CODE (nntp, MU_NNTP_RESP_CODE_NEWNEWS_FOLLOW);

      status = mu_nntp_iterator_create (nntp, piterator);
      MU_NNTP_CHECK_ERROR(nntp, status);
      nntp->state = MU_NNTP_NEWNEWS_RX;

    case MU_NNTP_NEWNEWS_RX:
      break;

      /* They must deal with the error first by reopening.  */
    case MU_NNTP_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }

  return status;
}

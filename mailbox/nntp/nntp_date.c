/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <mailutils/sys/nntp.h>

int
mu_nntp_date (mu_nntp_t nntp, unsigned int *year, unsigned int *month, unsigned int *day,
	      unsigned int *hour, unsigned int *min, unsigned int *sec)
{
  int status;
  unsigned long dummy = 0;
  char *buf;

  if (nntp == NULL)
    return EINVAL;

  switch (nntp->state)
    {
    case MU_NNTP_NO_STATE:
      status = mu_nntp_writeline (nntp, "DATE\r\n");
      MU_NNTP_CHECK_ERROR (nntp, status);
      mu_nntp_debug_cmd (nntp);
      nntp->state = MU_NNTP_DATE;

    case MU_NNTP_DATE:
      status = mu_nntp_send (nntp);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->acknowledge = 0;
      nntp->state = MU_NNTP_DATE_ACK;

    case MU_NNTP_DATE_ACK:
      status = mu_nntp_response (nntp, NULL, 0, NULL);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      mu_nntp_debug_ack (nntp);
      MU_NNTP_CHECK_CODE(nntp, MU_NNTP_RESP_CODE_SERVER_DATE);
      nntp->state = MU_NNTP_NO_STATE;

      /* parse the answer now. */
      status = mu_parse_date(nntp, MU_NNTP_RESP_CODE_SERVER_DATE, year, month, day, hour, min, sec);
      MU_NNTP_CHECK_ERROR (nntp, status);
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

static int
mu_nntp_parse_date (mu_nntp_t nntp, int code, unsigned int *year, unsigned int *month, unsigned int *day,
		    unsigned int *hour, unsigned int *min, unsigned int *sec)
{
  unsigned int dummy = 0;
  char *buf;
  char format[32];

  if (year == NULL)
    year = &dummy;
  if (month == NULL)
    month = &dummy;
  if (day == NULL)
    day = &dummy;
  if (hour == NULL)
    hour = &dummy;
  if (min == NULL)
    min = &dummy;
  if (sec == NULL)
    sec = &dummy;

  sprintf (format, "%d %%4d%%2d%%2d%%2d%%2d%%2d", code);
  sscanf (nntp->ack.buf, format, year, month, day, hour, min, sec);
  return 0;
}

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
mu_nntp_head (mu_nntp_t nntp, unsigned long number, unsigned long *pnum, char **mid, stream_t *pstream)
{
  int status;
  char *message_id = NULL;
  if (number != 0)
    {
      message_id = malloc (128);
      if (message_id == NULL)
	{
	  return ENOMEM;
	}
      snprintf (message_id, 127, "%d", number);
    }
  status = mu_nntp_head_id (nntp, message_id, pnum, mid, pstream);
  if (message_id)
    {
      free (message_id);
    }
  return status;
}

int
mu_nntp_head_id (mu_nntp_t nntp, const char *message_id, unsigned long *pnum, char **mid, stream_t *pstream)
{
  int status;
  unsigned long dummy = 0;
  char *buf;

  if (nntp == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  switch (nntp->state)
    {
    case MU_NNTP_NO_STATE:
      if (message_id == NULL || *message_id == '\0')
	{
	  status = mu_nntp_writeline (nntp, "HEAD\r\n");
	}
      else
	{
	  status = mu_nntp_writeline (nntp, "HEAD %s\r\n", message_id);
	}
      MU_NNTP_CHECK_ERROR (nntp, status);
      mu_NNTP_debug_cmd (nntp);
      nntp->state = MU_NNTP_HEAD;

    case MU_NNTP_HEAD:
      status = mu_nntp_send (nntp);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->acknowledge = 0;
      nntp->state = MU_NNTP_HEAD_ACK;

    case MU_NNTP_HEAD_ACK:
      status = mu_nntp_response (nntp, NULL, 0, NULL);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      mu_nntp_debug_ack (nntp);
      MU_NNTP_CHECK_OK (nntp);
      nntp->state = MU_NNTP_HEAD_RX;
      /* parse the answer now. */
      if (pnum == NULL)
	{
	  pnum = &dummy;
	}
      buf = calloc(sizeof(*buf), 128);
      if (buf == NULL)
	{
	  return ENOMEM;
	}
      sscanf (nntp->ack.buf, "221 %d %127s", pnum, buf);
      if (*buf == '\0')
	{
	  strcpy(buf, "<0>");
	}
      if (mid)
	{
	  *mid = buf;
	}
      else
	{
	  free (buf);
	}

    case MU_NNTP_HEAD_RX:
      status = mu_nntp_stream_create (nntp, pstream);
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

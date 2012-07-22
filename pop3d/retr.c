/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "pop3d.h"

size_t pop3d_output_bufsize = 64 * 1024;

void
pop3d_send_payload (mu_stream_t stream, mu_stream_t linestr, size_t maxlines)
{
  mu_stream_t flt;
  struct mu_buffer_query oldbuf, newbuf;
  int xscript_level = set_xscript_level (MU_XSCRIPT_PAYLOAD);

  oldbuf.type = MU_TRANSPORT_OUTPUT;
  mu_stream_ioctl (iostream, MU_IOCTL_TRANSPORT_BUFFER, MU_IOCTL_OP_GET,
		   &oldbuf);
  newbuf.type = MU_TRANSPORT_OUTPUT;
  newbuf.buftype = mu_buffer_full;
  newbuf.bufsize = pop3d_output_bufsize;
  mu_stream_ioctl (iostream, MU_IOCTL_TRANSPORT_BUFFER, MU_IOCTL_OP_SET,
		   &newbuf);
  /* FIXME: Return code */
  mu_filter_create (&flt, iostream, "DOT", MU_FILTER_ENCODE,
		    MU_STREAM_WRITE);
  
  mu_stream_copy (flt, stream, 0, NULL);

  if (maxlines)
    {
      char *buf = NULL;
      size_t size = 0, n;

      mu_stream_write (flt, "\n", 1, NULL);
      for (; maxlines > 0 &&
	     mu_stream_getline (linestr, &buf, &size, &n) == 0 &&
	     n > 0; maxlines--)
	mu_stream_write (flt, buf, n, NULL);
      free (buf);
    }
  
  mu_stream_close (flt);
  mu_stream_destroy (&flt);
  
  mu_stream_ioctl (iostream, MU_IOCTL_TRANSPORT_BUFFER, MU_IOCTL_OP_SET,
		   &oldbuf);
  set_xscript_level (xscript_level);
}

/* Prints out the specified message */

int
pop3d_retr (char *arg, struct pop3d_session *sess)
{
  size_t mesgno;
  mu_message_t msg = NULL;
  mu_attribute_t attr = NULL;
  mu_stream_t stream;
  
  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesgno = strtoul (arg, NULL, 10);

  if (mu_mailbox_get_message (mbox, mesgno, &msg) != 0)
    return ERR_NO_MESG;

  mu_message_get_attribute (msg, &attr);
  if (pop3d_is_deleted (attr))
    return ERR_MESG_DELE;

  if (mu_message_get_streamref (msg, &stream))
    return ERR_UNKNOWN;
  
  pop3d_outf ("+OK\n");
  pop3d_send_payload (stream, NULL, 0);
  mu_stream_destroy (&stream);
  
  if (!mu_attribute_is_read (attr))
    mu_attribute_set_read (attr);

  pop3d_mark_retr (attr);

  
  return OK;
}

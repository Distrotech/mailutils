/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "pop3d.h"

/* Prints out the specified message */

int
pop3d_retr (const char *arg)
{
  size_t mesgno, n;
  char *buf;
  size_t buflen = BUFFERSIZE;
  mu_message_t msg = NULL;
  mu_attribute_t attr = NULL;
  mu_stream_t stream = NULL;
  off_t off;

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

  mu_message_get_stream (msg, &stream);
  pop3d_outf ("+OK\r\n");

  off = n = 0;
  buf = malloc (buflen * sizeof (*buf));
  if (buf == NULL)
    pop3d_abquit (ERR_NO_MEM);

  while (mu_stream_readline (stream, buf, buflen, off, &n) == 0
	 && n > 0)
    {
      /* Nuke the trailing newline.  */
      if (buf[n - 1] == '\n')
	buf[n - 1] = '\0';
      else /* Make room for the line.  */
	{
	  buflen *= 2;
	  buf = realloc (buf, buflen * sizeof (*buf));
	  if (buf == NULL)
	    pop3d_abquit (ERR_NO_MEM);
	  continue;
	}
      if (buf[0] == '.')
	pop3d_outf (".%s\r\n", buf);
      else
	pop3d_outf ("%s\r\n", buf);
      off += n;
    }

  if (!mu_attribute_is_read (attr))
    mu_attribute_set_read (attr);

  pop3d_mark_retr (attr);

  free (buf);
  pop3d_outf (".\r\n");

  return OK;
}

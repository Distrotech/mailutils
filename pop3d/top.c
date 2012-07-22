/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
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

/* Prints the header of a message plus a specified number of lines.  */

int
pop3d_top (char *arg, struct pop3d_session *sess)
{
  size_t mesgno;
  unsigned long lines;
  mu_message_t msg;
  mu_attribute_t attr;
  mu_header_t hdr;
  mu_body_t body;
  mu_stream_t hstream, bstream;
  char *mesgc, *linesc, *p;
  
  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  pop3d_parse_command (arg, &mesgc, &linesc);
  if (linesc[0] == 0)
    return ERR_BAD_ARGS;
  
  mesgno = strtoul (mesgc, &p, 10);
  if (*p)
    return ERR_BAD_ARGS;
  lines = strtoul (linesc, &p, 10);
  if (*p)
    return ERR_BAD_ARGS;

  if (mu_mailbox_get_message (mbox, mesgno, &msg) != 0)
    return ERR_NO_MESG;

  mu_message_get_attribute (msg, &attr);
  if (pop3d_is_deleted (attr))
    return ERR_MESG_DELE;
  pop3d_mark_retr (attr);
  
  /* Header.  */
  mu_message_get_header (msg, &hdr);
  if (mu_header_get_streamref (hdr, &hstream))
    return ERR_UNKNOWN;
  mu_message_get_body (msg, &body);
  if (mu_body_get_streamref (body, &bstream))
    {
      mu_stream_unref (hstream);
      return ERR_UNKNOWN;
    }
    
  pop3d_outf ("+OK\n");

  pop3d_send_payload (hstream, bstream, lines);

  mu_stream_unref (hstream);
  mu_stream_unref (bstream);

  return OK;
}

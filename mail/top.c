/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/*
 * to[p] [msglist]
 */

static int
top0 (msgset_t *mspec, message_t msg, void *data)
{
  stream_t stream;
  char buf[512];
  size_t n;
  off_t off;
  int lines;
  attribute_t attr = NULL;
  
  if (util_getenv (&lines, "toplines", Mail_env_number, 1)
      || lines < 0)
    return 1;

  message_get_stream (msg, &stream);
  for (n = 0, off = 0; lines > 0; lines--, off += n)
    {
      int status = stream_readline (stream, buf, sizeof (buf), off, &n);
      if (status != 0 || n == 0)
	break;
      fprintf (ofile, "%s", buf);
    }
  cursor = mspec->msg_part[0];

  message_get_attribute (msg, &attr);
  attribute_set_read (attr);
  attribute_set_userflag (attr, MAIL_ATTRIBUTE_SHOWN);
  return 0;
}

int
mail_top (int argc, char **argv)
{
  return util_foreach_msg (argc, argv, MSG_NODELETED, top0, NULL);
}


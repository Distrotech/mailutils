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

#include "mail.h"

/*
 * to[p] [msglist]
 */

static int
top0 (msgset_t *mspec, mu_message_t msg, void *data)
{
  mu_stream_t stream;
  char *buf = NULL;
  size_t size = 0, n;
  int lines;
  
  if (mailvar_get (&lines, "toplines", mailvar_type_number, 1)
      || lines < 0)
    return 1;

  mu_message_get_streamref (msg, &stream);
  for (; lines > 0; lines--)
    {
      int status = mu_stream_getline (stream, &buf, &size, &n);
      if (status != 0 || n == 0)
	break;
      mu_printf ("%s", buf);
    }
  free (buf);
  mu_stream_destroy (&stream);
  set_cursor (mspec->msg_part[0]);

  util_mark_read (msg);

  return 0;
}

int
mail_top (int argc, char **argv)
{
  return util_foreach_msg (argc, argv, MSG_NODELETED, top0, NULL);
}


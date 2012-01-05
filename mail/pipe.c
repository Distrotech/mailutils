/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2003, 2005, 2007, 2010-2012 Free Software
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
 * pi[pe] [[msglist] command]
 * | [[msglist] command]
 */

int
mail_pipe (int argc, char **argv)
{
  int rc;
  mu_stream_t outstr;
  char *cmd;
  msgset_t *list, *mp;

  if (argc > 2)
    cmd = argv[--argc];
  else if (mailvar_get (&cmd, "cmd", mailvar_type_string, 1))
    return 1;

  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &list))
    return 1;

  rc = mu_command_stream_create (&outstr, cmd, MU_STREAM_WRITE);
  if (rc)
    {
      mu_error (_("cannot open `%s': %s"), cmd, mu_strerror (rc));
      return 1;
    }

  for (mp = list; mp; mp = mp->next)
    {
      mu_message_t msg;
      mu_stream_t stream;
      
      if (util_get_message (mbox, mp->msg_part[0], &msg) == 0)
	{
	  mu_message_get_streamref (msg, &stream);
	  mu_stream_copy (outstr, stream, 0, NULL);
	  mu_stream_destroy (&stream);
	  if (mailvar_get (NULL, "page", mailvar_type_boolean, 0) == 0)
	    mu_stream_write (outstr, "\f\n", 2, NULL);
	}
      util_mark_read (msg);
    }
  mu_stream_close (outstr);
  mu_stream_destroy (&outstr);
  msgset_free (list);

  return 0;
}

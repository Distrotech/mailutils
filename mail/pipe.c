/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2003 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

/*
 * pi[pe] [[msglist] command]
 * | [[msglist] command]
 */

int
mail_pipe (int argc, char **argv)
{
  message_t msg;
  stream_t stream;
  char *cmd;
  FILE *tube;
  msgset_t *list, *mp;
  char buffer[512];
  off_t off = 0;
  size_t n = 0;

  if (argc > 1)
    cmd = argv[--argc];
  else if (util_getenv (&cmd, "cmd", Mail_env_boolean, 1))
    return 1;

  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &list))
      return 1;

  tube = popen (cmd, "w");

  for (mp = list; mp; mp = mp->next)
    {
      if (util_get_message (mbox, mp->msg_part[0], &msg) == 0)
	{
	  message_get_stream (msg, &stream);
	  off = 0;
	  while (stream_read (stream, buffer, sizeof (buffer) - 1, off,
			      &n) == 0 && n != 0)
	    {
	      buffer[n] = '\0';
	      fprintf (tube, "%s", buffer);
	      off += n;
	    }
	  if (util_getenv (NULL, "page", Mail_env_boolean, 0) == 0)
	    fprintf (tube, "\f\n");
	}
    }

  msgset_free (list);
  pclose (tube);
  return 0;
}

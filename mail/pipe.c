/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
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
  FILE *pipe;
  int *list, num = 0;
  char buffer[512];
  off_t off = 0;
  size_t n = 0;

  if (argc > 1)
    cmd = argv[--argc];
  else if ((util_find_env ("cmd"))->set)
    cmd = (util_find_env ("cmd"))->value;
  else
    return 1;

  pipe = popen (cmd, "w");
  if ((num = util_expand_msglist (argc, argv, &list)) > 0)
    {
      int i = 0;
      for (i = 0; i < num; i++)
	{
	  if (mailbox_get_message (mbox, list[i], &msg) == 0)
	    {
	      message_get_stream (msg, &stream);
	      off = 0;
	      while (stream_read (stream, buffer, sizeof (buffer) - 1, off,
				  &n) == 0 && n != 0)
		{
		  buffer[n] = '\0';
		  fprintf (pipe, "%s", buffer);
		  off += n;
		}
	      if ((util_find_env("page"))->set && i < num - 1)
		fprintf (pipe, "\f\n");
	    }
	}
    }
  else if (mailbox_get_message (mbox, cursor, &msg) == 0)
    {
      message_get_stream (msg, &stream);
      while (stream_read (stream, buffer, sizeof (buffer) - 1, off, &n) == 0
	     && n != 0)
	{
	  buffer[n] = '\0';
	  fprintf (pipe, "%s", buffer);
	  off += n;
	}
    }

  free (list);
  pclose (pipe);
  return 0;
}

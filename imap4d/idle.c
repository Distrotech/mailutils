/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012 Free Software Foundation, Inc.

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

#include "imap4d.h"

int
imap4d_idle (struct imap4d_session *session,
             struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  time_t start;
  char *token_str = NULL;
  size_t token_size = 0, token_len;
  
  if (imap4d_tokbuf_argc (tok) != 2)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");

  if (io_wait_input (0) == -1)
    return io_completion_response (command, RESP_NO, "Cannot idle");

  io_sendf ("+ idling\n");
  io_flush ();

  start = time (NULL);
  while (1)
    {
      if (io_wait_input (5))
	{
          io_getline (&token_str, &token_size, &token_len); 	  
	  if (token_len == 4 && mu_c_strcasecmp (token_str, "done") == 0)
	    break;
	}
      else if (time (NULL) - start > idle_timeout)
	imap4d_bye (ERR_TIMEOUT);

      imap4d_sync ();
      io_flush ();
    }
  free (token_str);
  return io_completion_response (command, RESP_OK, "terminated");
}


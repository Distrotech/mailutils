/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#include "imap4d.h"

int
imap4d_idle (struct imap4d_command *command, char *arg)
{
  char *sp;
  time_t start;
  
  if (util_getword (arg, &sp))
    return util_finish (command, RESP_BAD, "Too many args");
  
  if (util_wait_input (0) == -1)
    return util_finish (command, RESP_NO, "Cannot idle");

  util_send ("+ idling\r\n");
  util_flush_output ();

  start = time (NULL);
  while (1)
    {
      if (util_wait_input (5))
	{
	  int rc;
	  char *p;
	  char *cmd = imap4d_readline ();
	  
	  p = util_getword (cmd, &sp);
	  rc = strcasecmp (p, "done") == 0;

	  free (cmd);
	  if (rc)
	    break;
	}
      else if (time (NULL) - start > daemon_param.timeout)
	imap4d_bye (ERR_TIMEOUT);

      imap4d_sync ();
      util_flush_output ();
    }

  return util_finish (command, RESP_OK, "terminated");
}


/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include "imap4d.h"

int ifile;
int ofile;

int
main (int argc, char **argv)
{
  chdir ("/");
  openlog ("imap4d", LOG_PID, LOG_MAIL);

  /* for each incoming connection */
  {
    char *remote_host = "";
    ifile = fileno (stdin);
    ofile = fileno (stdout);

    syslog (LOG_INFO, "Incoming connection from %s", remote_host);
    util_out (NULL, TAG_NONE, "IMAP4rev1 GNU " PACKAGE " " VERSION);
    while (1)
      {
	char *cmd = util_getline ();
	/* check for updates */
	util_do_command (cmd);
	free (cmd);
      }
  }

  closelog ();
  return 0;
}

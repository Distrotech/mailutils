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

#include "imap4d.h"

FILE *ofile;
unsigned int timeout = 1800; /* RFC2060: 30 minutes, if enable.  */
mailbox_t mbox;
char *homedir;
int state = STATE_NONAUTH;

static int imap4d_mainloop __P ((int, int));

int
main (int argc, char **argv)
{
  chdir ("/");
  openlog ("imap4d", LOG_PID, LOG_MAIL);

  /* Register the desire formats. We only need Mbox mail format.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, path_record);
  }
  /* FIXME: Incomplete, make it work for standalone, see pop3d.  */
  imap4d_mainloop (fileno (stdin), fileno (stdout));
  return 0;
}

static int
imap4d_mainloop (int infile, int outfile)
{
  const char *remote_host = "";
  FILE *ifile;
  ifile = fdopen (infile, "r");
  ofile = fdopen (outfile, "w");
  if (!ofile || !ifile)
    util_quit (ERR_NO_OFILE);

  /* FIXME: Retreive hostname with getpeername() and log.  */
  syslog (LOG_INFO, "Incoming connection from %s", remote_host);

  /* Greetings.  */
  util_out (RESP_OK, "IMAP4rev1 GNU " PACKAGE " " VERSION);
  fflush (ofile);

  while (1)
    {
      char *cmd = imap4d_readline (ifile);
      /* check for updates */
      imap4d_sync ();
      util_do_command (cmd);
      imap4d_sync ();
      free (cmd);
      fflush (ofile);
    }

  closelog ();
  return 0;
}

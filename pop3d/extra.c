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

#include "pop3d.h"

/* Takes a string as input and returns either the remainder of the string
   after the first space, or a zero length string if no space */

char *
pop3d_args (const char *cmd)
{
  int space = -1, i = 0, len;
  char *buf;

  len = strlen (cmd) + 1;
  buf = malloc (len * sizeof (char));
  if (buf == NULL)
    pop3d_abquit (ERR_NO_MEM);

  while (space < 0 && i < len)
    {
      if (cmd[i] == ' ')
	space = i + 1;
      else if (cmd[i] == '\0' || cmd[i] == '\r' || cmd[i] == '\n')
	len = i;
      i++;
    }

  if (space < 0)
    buf[0] = '\0';
  else
    {
      for (i = space; i < len; i++)
	if (cmd[i] == '\0' || cmd[i] == '\r' || cmd[i] == '\n')
	  buf[i - space] = '\0';
	else
	  buf[i - space] = cmd[i];
    }

  return buf;
}

/* This takes a string and returns the string up to the first space or end of
   the string, whichever occurs first */

char *
pop3d_cmd (const char *cmd)
{
  char *buf;
  int i = 0, len;

  len = strlen (cmd) + 1;
  buf = malloc (len * sizeof (char));
  if (buf == NULL)
    pop3d_abquit (ERR_NO_MEM);

  for (i = 0; i < len; i++)
    {
      if (cmd[i] == ' ' || cmd[i] == '\0' || cmd[i] == '\r' || cmd[i] == '\n')
	len = i;
      else
	buf[i] = cmd[i];
    }
  buf[i - 1] = '\0';
  return buf;
}

/* This is called if GNU POP3 needs to quit without going to the UPDATE stage.
   This is used for conditions such as out of memory, a broken socket, or
   being killed on a signal */
int
pop3d_abquit (int reason)
{
  /* Unlock spool */
  pop3d_unlock();
  mailbox_close (mbox);
  mailbox_destroy (&mbox);

  switch (reason)
    {
    case ERR_NO_MEM:
      fprintf (ofile, "-ERR Out of memory, quitting\r\n");
      syslog (LOG_ERR, "Out of memory");
      break;

    case ERR_SIGNAL:
      fprintf (ofile, "-ERR Quitting on signal\r\n");
      syslog (LOG_ERR, "Quitting on signal");
      break;

    case ERR_TIMEOUT:
      fprintf (ofile, "-ERR Session timed out\r\n");
      if (state == TRANSACTION)
	syslog (LOG_INFO, "Session timed out for user: %s", username);
      else
	syslog (LOG_INFO, "Session timed out for no user");
      break;

    case ERR_NO_OFILE:
      syslog (LOG_INFO, "No socket to send to");
      break;

    case ERR_MBOX_SYNC:
      syslog (LOG_ERR, "Mailbox was updated by other party: %s", username);
      fprintf (ofile, "-ERR [OUT-SYNC] Mailbox updated by other party or corrupt\r\n");
      break;

    default:
      fprintf (ofile, "-ERR Quitting (reason unknown)\r\n");
      syslog (LOG_ERR, "Unknown quit");
      break;
    }

  closelog();
  exit (1);
}

/* Prints out usage information and exits the program */

void
pop3d_usage (char *argv0)
{
  printf ("Usage: %s [OPTIONS]\n", argv0);
  printf ("Runs the GNU POP3 daemon.\n\n");
  printf ("  -d, --daemon=MAXCHILDREN runs in daemon mode with a maximum\n");
  printf ("                           of MAXCHILDREN child processes\n");
  printf ("  -h, --help               display this help and exit\n");
  printf ("  -i, --inetd              runs in inetd mode (default)\n");
  printf ("  -p, --port=PORT          specifies port to listen on, implies -d\n");
  printf ("                           defaults to 110, which need not be specified\n");
  printf ("  -t, --timeout=TIMEOUT    sets idle timeout to TIMEOUT seconds\n");
  printf ("                           TIMEOUT default is 600 (10 minutes)\n");
  printf ("  -v, --version            display version information and exit\n");
  printf ("\nReport bugs to bug-mailutils@gnu.org\n");
  exit (0);
}

/* Default signal handler to call the pop3d_abquit() function */

RETSIGTYPE
pop3d_signal (int signo)
{
  syslog (LOG_CRIT, "got signal %d", signo);
  /* Master process.  */
  if (!ofile)
    {
       syslog(LOG_CRIT, "MASTER: exiting on signal");
       exit (1); /* abort(); */
    }

  if (signo == SIGALRM)
    pop3d_abquit (ERR_TIMEOUT);
  pop3d_abquit (ERR_SIGNAL);
}

/* Gets a line of input from the client, caller should free() */
char *
pop3d_readline (FILE *fp)
{
  static char buffer[512];
  char *ptr;

  alarm (timeout);
  ptr = fgets (buffer, sizeof (buffer), fp);
  alarm (0);

  /* We should probably check ferror() too, but if ptr is null we
     are done anyway;  if (!ptr && ferror(fp)) */
  if (!ptr)
    pop3d_abquit (ERR_NO_OFILE);

  /* Caller should not free () this ... should we strdup() then?  */
  return ptr;
}

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
  (void)signo;
  syslog (LOG_CRIT, "got signal %d", signo);
  pop3d_abquit (ERR_SIGNAL);
}

/* Gets a line of input from the client */
/* We can also implement PIPELINING by keeping a static buffer.
   Implementing this cost an extra allocation with more uglier code.
   Is it worth it?  How many clients actually use PIPELINING?
 */
char *
pop3d_readline (int fd)
{
  static char *buffer = NULL; /* Note: This buffer is never free()d.  */
  static size_t total = 0;
  char *nl;
  char *line;
  size_t len;

  nl = memchr (buffer, '\n', total);
  if (!nl)
    {
      /* Need to refill the buffer.  */
      do
	{
	  char buf[512];
	  int nread;

	  if (timeout)
	    {
	      int available;
	      fd_set rfds;
	      struct timeval tv;

	      FD_ZERO (&rfds);
	      FD_SET (fd, &rfds);
	      tv.tv_sec = timeout;
	      tv.tv_usec = 0;

	      available = select (fd + 1, &rfds, NULL, NULL, &tv);
	      if (!available)
		pop3d_abquit (ERR_TIMEOUT);
	      else if (available == -1)
		{
		  if (errno == EINTR)
		    continue;
		  pop3d_abquit (ERR_NO_OFILE);
		}
	    }

	  errno = 0;
	  nread = read (fd, buf, sizeof (buf) - 1);
	  if (nread < 1)
	    {
	      if (errno == EINTR)
		continue;
	      pop3d_abquit (ERR_NO_OFILE);
	    }
	  buf[nread] = '\0';

	  buffer = realloc (buffer, (total + nread + 1) * sizeof (*buffer));
	  if (buffer == NULL)
	    pop3d_abquit (ERR_NO_MEM);
	  memcpy (buffer + total, buf, nread + 1); /* copy the null too.  */
	  total += nread;
	}
      while ((nl = memchr (buffer, '\n', total)) == NULL);
    }

  nl++;
  len = nl - buffer;
  line = calloc (len + 1, sizeof (*line));
  memcpy (line, buffer, len); /* copy the newline too.  */
  line[len] = '\0';
  total -= len;
  memmove (buffer, nl, total);
  return line;
}

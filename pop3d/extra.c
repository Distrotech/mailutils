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

#include "pop3d.h"

/* Takes a string as input and returns either the remainder of the string
   after the first space, or a zero length string if no space */

char *
pop3_args (const char *cmd)
{
  int space = -1, i = 0, len;
  char *buf;

  len = strlen (cmd) + 1;
  buf = malloc (len * sizeof (char));
  if (buf == NULL)
    pop3_abquit (ERR_NO_MEM);

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
pop3_cmd (const char *cmd)
{
  char *buf;
  int i = 0, len;

  len = strlen (cmd) + 1;
  buf = malloc (len * sizeof (char));
  if (buf == NULL)
    pop3_abquit (ERR_NO_MEM);

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
pop3_abquit (int reason)
{
  mbox_close(mbox);
  
  switch (reason)
    {
    case ERR_NO_MEM:
      fprintf (ofile, "-ERR Out of memory, quitting\r\n");
      syslog (LOG_ERR, "Out of memory");
      break;
    case ERR_DEAD_SOCK:
      fprintf (ofile, "-ERR Socket closed, quitting\r\n");
      syslog (LOG_ERR, "Socket closed");
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
    default:
      fprintf (ofile, "-ERR Quitting (reason unknown)\r\n");
      syslog (LOG_ERR, "Unknown quit");
      break;
    }
  fflush (ofile);
  closelog();
  exit (1);
}

/* Prints out usage information and exits the program */

void
pop3_usage (char *argv0)
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
  printf ("                           TIMEOUT must be at least 600 (10 minutes) or\n");
  printf ("                           it will be disabled\n");
  printf ("  -v, --version            display version information and exit\n");
  printf ("\nReport bugs to gnu-pop-list@nodomainname.net\n");
  exit (0);
}

/* Default signal handler to call the pop3_abquit() function */

void
pop3_signal (int signal)
{
  pop3_abquit (ERR_SIGNAL);
}

/* Gets a line of input from the client */

char *
pop3_readline (int fd)
{
  fd_set rfds;
  struct timeval tv;
  char buf[1024], *ret = NULL;
  int available;

  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  memset (buf, '\0', 1024);

  do
    {
      if (timeout > 0)
	{
	  available = select (fd + 1, &rfds, NULL, NULL, &tv);
	  if (!available)
	    pop3_abquit (ERR_TIMEOUT);
	}

      if (read (fd, buf, 1024) < 1)
	pop3_abquit (ERR_DEAD_SOCK);

      if (ret == NULL)
	{
	  ret = malloc ((strlen (buf) + 1) * sizeof (char));
	  strcpy (ret, buf);
	}
      else
	{
	  ret = realloc (ret, (strlen (ret) + strlen (buf) + 1) * sizeof (char));
	  strcat (ret, buf);
	}
    }
  while (strchr (buf, '\n') == NULL);

  return ret;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003 Free Software Foundation, Inc.

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

#include "pop3d.h"

static FILE *ifile;
static FILE *ofile;
#ifdef WITH_TLS
static gnutls_session sfile;
#endif /* WITH_TLS */

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
  pop3d_unlock ();
  mailbox_close (mbox);
  mailbox_destroy (&mbox);

  switch (reason)
    {
    case ERR_NO_MEM:
      pop3d_outf ("-ERR Out of memory, quitting\r\n");
      syslog (LOG_ERR, _("Out of memory"));
      break;

    case ERR_SIGNAL:
      pop3d_outf ("-ERR Quitting on signal\r\n");
      syslog (LOG_ERR, _("Quitting on signal"));
      break;

    case ERR_TIMEOUT:
      pop3d_outf ("-ERR Session timed out\r\n");
      if (state == TRANSACTION)
	syslog (LOG_INFO, _("Session timed out for user: %s"), username);
      else
	syslog (LOG_INFO, _("Session timed out for no user"));
      break;

    case ERR_NO_OFILE:
      syslog (LOG_INFO, _("No socket to send to"));
      break;

    case ERR_MBOX_SYNC:
      syslog (LOG_ERR, _("Mailbox was updated by other party: %s"), username);
      pop3d_outf
	("-ERR [OUT-SYNC] Mailbox updated by other party or corrupt\r\n");
      break;

    default:
      pop3d_outf ("-ERR Quitting (reason unknown)\r\n");
      syslog (LOG_ERR, _("Unknown quit"));
      break;
    }

  closelog ();
  exit (EXIT_FAILURE);
}

void
pop3d_setio (FILE * in, FILE * out)
{
  if (!in || !out)
    pop3d_abquit (ERR_NO_OFILE);

  ifile = in;
  ofile = out;
}

#ifdef WITH_TLS

int
pop3d_init_tls_server ()
{
  sfile =
    (gnutls_session) mu_init_tls_server (fileno (ifile), fileno (ofile));
  if (!sfile)
    return 0;
  return 1;
}

void
pop3d_deinit_tls_server ()
{
  mu_deinit_tls_server (sfile);
}

#endif /* WITH_TLS */

void
pop3d_flush_output ()
{
  fflush (ofile);
}

int
pop3d_is_master ()
{
  return ofile == NULL;
}

void
pop3d_outf (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  if (daemon_param.transcript)
    {
      char *buf;
      vasprintf (&buf, fmt, ap);
      if (buf)
	{
	  syslog (LOG_DEBUG, "sent: %s", buf);
	  free (buf);
	}
    }

#ifdef WITH_TLS
  if (tls_done)
    {
      char *buf;
      vasprintf (&buf, fmt, ap);
      if (buf)
	{
	  gnutls_record_send (sfile, buf, strlen (buf));
	  free (buf);
	}
    }
  else
#endif /* WITH_TLS */
    vfprintf (ofile, fmt, ap);

  va_end (ap);
}


/* Gets a line of input from the client, caller should free() */
char *
pop3d_readline (char *buffer, size_t size)
{
  char *ptr;

  alarm (daemon_param.timeout);
#ifdef WITH_TLS
  if (tls_done)
    {
      int rc = gnutls_record_recv (sfile, buffer, size - 1);
      if (rc < 0)
	{
	  syslog (LOG_ERR, _("TLS error on read: %s"),
		  gnutls_strerror (rc));
	  pop3d_abquit (ERR_TLS_IO);
	}
      else
	buffer[rc] = 0;
      ptr = buffer;
    }
  else
#endif /* WITH_TLS */
    ptr = fgets (buffer, size, ifile);
  alarm (0);

  /* We should probably check ferror() too, but if ptr is null we
     are done anyway;  if (!ptr && ferror(ifile)) */
  if (!ptr)
    pop3d_abquit (ERR_NO_OFILE);

  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "recv: %s", ptr);

  /* Caller should not free () this ... should we strdup() then?  */
  return ptr;
}

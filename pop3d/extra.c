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

static stream_t istream, ostream;

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
pop3d_setio (FILE *in, FILE *out)
{
  if (!in || !out)
    pop3d_abquit (ERR_NO_OFILE);

  if (stdio_stream_create (&istream, in, MU_STREAM_NO_CLOSE)
      || stdio_stream_create (&ostream, out, MU_STREAM_NO_CLOSE))
    pop3d_abquit (ERR_NO_OFILE);
}

#ifdef WITH_TLS
int
pop3d_init_tls_server ()
{
  stream_t stream;
  int in_fd;
  int out_fd;
  int rc;
  
  if (stream_get_fd (istream, &in_fd)
      || stream_get_fd (ostream, &out_fd))
    return 0;
  rc = tls_stream_create (&stream, in_fd, out_fd, 0);
  if (rc)
    return 0;

  if (stream_open (stream))
    {
      const char *p;
      stream_strerror (stream, &p);
      syslog (LOG_ERR, _("cannot open TLS stream: %s"), p);
      return 0;
    }
  
  stream_destroy (&istream, stream_get_owner (istream));
  stream_destroy (&ostream, stream_get_owner (ostream));
  istream = ostream = stream;
  return 1;
}
#endif

void
pop3d_bye ()
{
  if (istream == ostream)
    {
      stream_close (istream);
      stream_destroy (&istream, stream_get_owner (istream));
    }
  /* There's no reason closing in/out streams otherwise */
#ifdef WITH_TLS
  mu_deinit_tls_libs ();
#endif /* WITH_TLS */
}

void
pop3d_flush_output ()
{
  stream_flush (ostream);
}

int
pop3d_is_master ()
{
  return ostream == NULL;
}

void
pop3d_outf (const char *fmt, ...)
{
  va_list ap;
  char *buf;
  int rc;
  
  va_start (ap, fmt);
  vasprintf (&buf, fmt, ap);
  va_end (ap);

  if (!buf)
    pop3d_abquit (ERR_NO_MEM);
  
  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "sent: %s", buf);

  rc = stream_sequential_write (ostream, buf, strlen (buf));
  free (buf);
  if (rc)
    {
      const char *p;

      if (stream_strerror (ostream, &p))
	p = strerror (errno);
      syslog (LOG_ERR, _("write failed: %s"), p);
      pop3d_abquit (ERR_NO_OFILE);
    }
}


/* Gets a line of input from the client, caller should free() */
char *
pop3d_readline (char *buffer, size_t size)
{
  int rc;
  size_t nbytes;
  
  alarm (daemon_param.timeout);
  rc = stream_sequential_readline (istream, buffer, size, &nbytes);
  alarm (0);

  if (rc)
    {
      const char *p;

      if (stream_strerror (ostream, &p))
	p = strerror (errno);
      syslog (LOG_ERR, _("write failed: %s"), p);
      pop3d_abquit (ERR_NO_OFILE);
    }

  if (daemon_param.transcript)
    syslog (LOG_DEBUG, "recv: %s", buffer);

  /* Caller should not free () this ... should we strdup() then?  */
  return buffer;
}

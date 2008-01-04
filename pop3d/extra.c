/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2005, 
   2007, 2008 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "pop3d.h"
#include "mailutils/libargp.h"

static mu_stream_t istream, ostream;

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
  if (state != AUTHORIZATION)
    {
      pop3d_unlock ();
      mu_mailbox_flush (mbox, 0);
      mu_mailbox_close (mbox);
      mu_mailbox_destroy (&mbox);
    }

  switch (reason)
    {
    case ERR_NO_MEM:
      pop3d_outf ("-ERR Out of memory, quitting\r\n");
      mu_diag_output (MU_DIAG_ERROR, _("Out of memory"));
      break;

    case ERR_SIGNAL:
      mu_diag_output (MU_DIAG_ERROR, _("Quitting on signal"));
      break;

    case ERR_TIMEOUT:
      pop3d_outf ("-ERR Session timed out\r\n");
      if (state == TRANSACTION)
	mu_diag_output (MU_DIAG_INFO, _("Session timed out for user: %s"), username);
      else
	mu_diag_output (MU_DIAG_INFO, _("Session timed out for no user"));
      break;

    case ERR_NO_OFILE:
      mu_diag_output (MU_DIAG_INFO, _("No socket to send to"));
      break;

    case ERR_MBOX_SYNC:
      mu_diag_output (MU_DIAG_ERROR, _("Mailbox was updated by other party: %s"), username);
      pop3d_outf
	("-ERR [OUT-SYNC] Mailbox updated by other party or corrupt\r\n");
      break;

    default:
      pop3d_outf ("-ERR Quitting (reason unknown)\r\n");
      mu_diag_output (MU_DIAG_ERROR, _("Quitting (numeric reason %d)"), reason);
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

  setvbuf (in, NULL, _IOLBF, 0);
  setvbuf (out, NULL, _IOLBF, 0);
  if (mu_stdio_stream_create (&istream, in, MU_STREAM_NO_CLOSE)
      || mu_stdio_stream_create (&ostream, out, MU_STREAM_NO_CLOSE))
    pop3d_abquit (ERR_NO_OFILE);
}

#ifdef WITH_TLS
int
pop3d_init_tls_server ()
{
  mu_stream_t stream;
  int rc;
 
  rc = mu_tls_stream_create (&stream, istream, ostream, 0);
  if (rc)
    return 0;

  if (mu_stream_open (stream))
    {
      const char *p;
      mu_stream_strerror (stream, &p);
      mu_diag_output (MU_DIAG_ERROR, _("cannot open TLS stream: %s"), p);
      return 0;
    }
  
  istream = ostream = stream;
  return 1;
}
#endif

void
pop3d_bye ()
{
  if (istream == ostream)
    {
      mu_stream_close (istream);
      mu_stream_destroy (&istream, mu_stream_get_owner (istream));
    }
  /* There's no reason closing in/out streams otherwise */
#ifdef WITH_TLS
  if (tls_available)
    mu_deinit_tls_libs ();
#endif /* WITH_TLS */
}

void
pop3d_flush_output ()
{
  mu_stream_flush (ostream);
}

int
pop3d_is_master ()
{
  return ostream == NULL;
}

static void
transcript (const char *pfx, const char *buf)
{
  if (pop3d_transcript)
    {
      int len = strlen (buf);
      if (len > 0 && buf[len-1] == '\n')
	{
	  len--;
	  if (len > 0 && buf[len-1] == '\r')
	    len--;
	}
      mu_diag_output (MU_DIAG_DEBUG, "%s: %-.*s", pfx, len, buf);
    }
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
  
  transcript ("sent", buf);

  rc = mu_stream_sequential_write (ostream, buf, strlen (buf));
  free (buf);
  if (rc)
    {
      const char *p;

      if (mu_stream_strerror (ostream, &p))
	p = strerror (errno);
      mu_diag_output (MU_DIAG_ERROR, _("Write failed: %s"), p);
      pop3d_abquit (ERR_NO_OFILE);
    }
}

/* Gets a line of input from the client, caller should free() */
char *
pop3d_readline (char *buffer, size_t size)
{
  int rc;
  size_t nbytes;
  
  alarm (idle_timeout);
  rc = mu_stream_sequential_readline (istream, buffer, size, &nbytes);
  alarm (0);

  if (rc)
    {
      const char *p;

      if (mu_stream_strerror (ostream, &p))
	p = strerror (errno);
      mu_diag_output (MU_DIAG_ERROR, _("Read failed: %s"), p);
      pop3d_abquit (ERR_NO_OFILE);
    }
  else if (nbytes == 0)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unexpected eof on input"));
      pop3d_abquit (ERR_NO_OFILE);
    }

  transcript ("recv", buffer);

  /* Caller should not free () this ... should we strdup() then?  */
  return buffer;
}


/* Handling of the deletion marks */

void
pop3d_mark_deleted (mu_attribute_t attr)
{
  mu_attribute_set_userflag (attr, POP3_ATTRIBUTE_DELE);
}

int
pop3d_is_deleted (mu_attribute_t attr)
{
  return mu_attribute_is_deleted (attr)
         || mu_attribute_is_userflag (attr, POP3_ATTRIBUTE_DELE);
}

void
pop3d_unset_deleted (mu_attribute_t attr)
{
  if (mu_attribute_is_userflag (attr, POP3_ATTRIBUTE_DELE))
    mu_attribute_unset_userflag (attr, POP3_ATTRIBUTE_DELE);
}

void
pop3d_undelete_all ()
{
  size_t i;
  size_t total = 0;

  mu_mailbox_messages_count (mbox, &total);

  for (i = 1; i <= total; i++)
    {
       mu_message_t msg = NULL;
       mu_attribute_t attr = NULL;
       mu_mailbox_get_message (mbox, i, &msg);
       mu_message_get_attribute (msg, &attr);
       mu_attribute_unset_deleted (attr);
    }
}

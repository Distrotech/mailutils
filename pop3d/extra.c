/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2003, 2005, 2007-2012 Free Software
   Foundation, Inc.

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

#include "pop3d.h"
#include "mailutils/libargp.h"

mu_stream_t iostream;

void
pop3d_parse_command (char *cmd, char **pcmd, char **parg)
{
  char *p;
  
  cmd = mu_str_skip_class (cmd, MU_CTYPE_BLANK);
  *pcmd = cmd;
  p = mu_str_skip_class_comp (cmd, MU_CTYPE_SPACE);
  *p++ = 0;
  if (*p)
    {
      *parg = p;
      mu_rtrim_class (p, MU_CTYPE_SPACE);
    }
  else
    *parg = "";
}

/* This is called if GNU POP3 needs to quit without going to the UPDATE stage.
   This is used for conditions such as out of memory, a broken socket, or
   being killed on a signal */
int
pop3d_abquit (int reason)
{
  int code;
  
  /* Unlock spool */
  if (state != AUTHORIZATION)
    {
      mu_mailbox_flush (mbox, 0);
      mu_mailbox_close (mbox);
      manlock_unlock (mbox);
      mu_mailbox_destroy (&mbox);
    }

  switch (reason)
    {
    case ERR_NO_MEM:
      code = EX_SOFTWARE;
      pop3d_outf ("-ERR %s\n", pop3d_error_string (reason));
      mu_diag_output (MU_DIAG_ERROR, _("not enough memory"));
      break;

    case ERR_SIGNAL:
      code = EX_SOFTWARE;
      mu_diag_output (MU_DIAG_ERROR, _("quitting on signal"));
      break;

    case ERR_TERMINATE:
      code = EX_OK;
      mu_diag_output (MU_DIAG_NOTICE, _("terminating on request"));
      break;

    case ERR_TIMEOUT:
      code = EX_TEMPFAIL;
      pop3d_outf ("-ERR %s\n", pop3d_error_string (reason));
      if (state == TRANSACTION)
	mu_diag_output (MU_DIAG_INFO, _("session timed out for user: %s"),
			username);
      else
	mu_diag_output (MU_DIAG_INFO, _("session timed out for no user"));
      break;

    case ERR_NO_IFILE:
      code = EX_NOINPUT;
      mu_diag_output (MU_DIAG_INFO, _("no input stream"));
      break;

    case ERR_NO_OFILE:
      code = EX_IOERR;
      mu_diag_output (MU_DIAG_INFO, _("no socket to send to"));
      break;

    case ERR_FILE:
      code = EX_IOERR;
      break;
      
    case ERR_PROTO:
      code = EX_PROTOCOL;
      mu_diag_output (MU_DIAG_INFO, _("remote protocol error"));
      break;

    case ERR_IO:
      code = EX_IOERR;
      mu_diag_output (MU_DIAG_INFO, _("I/O error"));
      break;

    case ERR_MBOX_SYNC:
      code = EX_OSERR; /* FIXME: This could be EX_SOFTWARE as well? */
      mu_diag_output (MU_DIAG_ERROR,
		      _("mailbox was updated by other party: %s"),
		      username);
      pop3d_outf
	("-ERR [OUT-SYNC] Mailbox updated by other party or corrupt\n");
      break;

    default:
      code = EX_SOFTWARE;
      pop3d_outf ("-ERR Quitting: %s\n", pop3d_error_string (reason));
      mu_diag_output (MU_DIAG_ERROR, _("quitting (numeric reason %d)"),
		      reason);
      break;
    }

  closelog ();
  exit (code);
}

void
pop3d_setio (int ifd, int ofd, int tls)
{
  mu_stream_t str, istream, ostream;
  
  if (ifd == -1)
    pop3d_abquit (ERR_NO_IFILE);
  if (ofd == -1)
    pop3d_abquit (ERR_NO_OFILE);

  if (mu_stdio_stream_create (&istream, ifd, MU_STREAM_READ))
    pop3d_abquit (ERR_NO_IFILE);
  mu_stream_set_buffer (istream, mu_buffer_line, 0);

  if (mu_stdio_stream_create (&ostream, ofd, MU_STREAM_WRITE))
    pop3d_abquit (ERR_NO_OFILE);

  /* Combine the two streams into an I/O one. */
#ifdef WITH_TLS
  if (tls)
    {
      int rc = mu_tls_server_stream_create (&str, istream, ostream, 0);
      if (rc)
	{
	  mu_stream_unref (istream);
	  mu_stream_unref (ostream);
	  mu_error (_("failed to create TLS stream: %s"), mu_strerror (rc));
	  pop3d_abquit (ERR_FILE);
	}
      tls_done = 1;
    }
  else
#endif
  if (mu_iostream_create (&str, istream, ostream))
    pop3d_abquit (ERR_FILE);

  /* Convert all writes to CRLF form.
     There is no need to convert reads, as the code ignores extra \r anyway.
  */
  if (mu_filter_create (&iostream, str, "CRLF", MU_FILTER_ENCODE,
			MU_STREAM_WRITE | MU_STREAM_RDTHRU))
    pop3d_abquit (ERR_NO_IFILE);
  /* Change buffering scheme: filter streams are fully buffered by default. */
  mu_stream_set_buffer (iostream, mu_buffer_line, 0);
  
  if (pop3d_transcript)
    {
      int rc;
      mu_stream_t dstr, xstr;
      
      rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
      if (rc)
	mu_error (_("cannot create debug stream; transcript disabled: %s"),
		  mu_strerror (rc));
      else
	{
	  rc = mu_xscript_stream_create (&xstr, iostream, dstr, NULL);
	  mu_stream_unref (dstr);
	  if (rc)
	    mu_error (_("cannot create transcript stream: %s"),
		      mu_strerror (rc));
	  else
	    {
	      mu_stream_unref (iostream);
	      iostream = xstr;
	    }
	}
    }
}

#ifdef WITH_TLS
int
pop3d_init_tls_server ()
{
  mu_stream_t tlsstream, stream[2];
  int rc;

  rc = mu_stream_ioctl (iostream, MU_IOCTL_SUBSTREAM, MU_IOCTL_OP_GET, stream);
  if (rc)
    {
      mu_error (_("%s failed: %s"), "MU_IOCTL_GET_STREAM",
		mu_stream_strerror (iostream, rc));
      return 1;
    }
  
  rc = mu_tls_server_stream_create (&tlsstream, stream[0], stream[1], 0);
  mu_stream_unref (stream[0]);
  mu_stream_unref (stream[1]);
  if (rc)
    return 1;

  stream[0] = stream[1] = tlsstream;
  rc = mu_stream_ioctl (iostream, MU_IOCTL_SUBSTREAM, MU_IOCTL_OP_SET, stream);
  mu_stream_unref (stream[0]);
  mu_stream_unref (stream[1]);
  if (rc)
    {
      mu_error (_("%s failed: %s"), "MU_IOCTL_SET_STREAM",
		mu_stream_strerror (iostream, rc));
      pop3d_abquit (ERR_IO);
    }
  return 0;
}
#endif

void
pop3d_bye ()
{
  mu_stream_close (iostream);
  mu_stream_destroy (&iostream);
#ifdef WITH_TLS
  if (tls_available)
    mu_deinit_tls_libs ();
#endif /* WITH_TLS */
}

void
pop3d_flush_output ()
{
  mu_stream_flush (iostream);
}

int
pop3d_is_master ()
{
  return iostream == NULL;
}

void
pop3d_outf (const char *fmt, ...)
{
  va_list ap;
  int rc;
  
  va_start (ap, fmt);
  rc = mu_stream_vprintf (iostream, fmt, ap);
  va_end (ap);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("Write failed: %s"),
 		      mu_stream_strerror (iostream, rc));
      pop3d_abquit (ERR_IO);
    }
}

/* Gets a line of input from the client, caller should free() */
char *
pop3d_readline (char *buffer, size_t size)
{
  int rc;
  size_t nbytes;
  
  alarm (idle_timeout);
  rc = mu_stream_readline (iostream, buffer, size, &nbytes);
  alarm (0);

  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("Read failed: %s"),
 		      mu_stream_strerror (iostream, rc));
      pop3d_abquit (ERR_IO);
    }
  else if (nbytes == 0)
    {
      /* After a failed authorization attempt many clients simply disconnect
	 without issuing QUIT. We do not count this as a protocol error. */
      if (state == AUTHORIZATION)
	exit (EX_OK);

      mu_diag_output (MU_DIAG_ERROR, _("Unexpected eof on input"));
      pop3d_abquit (ERR_PROTO);
    }

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

int
set_xscript_level (int xlev)
{
  if (pop3d_transcript)
    {
      if (xlev != MU_XSCRIPT_NORMAL)
	{
	  if (mu_debug_level_p (MU_DEBCAT_REMOTE,
	                        xlev == MU_XSCRIPT_SECURE ?
	                            MU_DEBUG_TRACE6 : MU_DEBUG_TRACE7))
	    return MU_XSCRIPT_NORMAL;
	}

      if (mu_stream_ioctl (iostream, MU_IOCTL_XSCRIPTSTREAM,
                           MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
	return xlev;
    }
  return 0;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2012 Free Software Foundation,
   Inc.

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

#include "imap4d.h"

int
imap4d_bye (int reason)
{
  return imap4d_bye0 (reason, NULL);
}

static jmp_buf pipejmp;

static RETSIGTYPE
sigpipe (int sig)
{
  longjmp (pipejmp, 1);
}

int
imap4d_bye0 (int reason, struct imap4d_command *command)
{
  int status = EX_SOFTWARE;

  /* Some clients may close the connection immediately after sending
     LOGOUT.  Do not treat this as error (RFC 2683).
     To be on the safe side, ignore broken pipes for any command: at
     this stage it is of no significance. */
  static int sigtab[] = { SIGPIPE };
  mu_set_signals (sigpipe, sigtab, MU_ARRAY_SIZE (sigtab));
  if (setjmp (pipejmp))
    {
      mu_set_signals (SIG_IGN, sigtab, MU_ARRAY_SIZE (sigtab));
      /* Invalidate iostream. This creates a mild memory leak, as the
	 stream is not destroyed by util_bye, but at least this avoids
	 endless loop which would happen when mu_stream_flush would
	 try to flush buffers on an already broken pipe.
	 FIXME: There must be a special function for that, I guess.
	 Something like mu_stream_invalidate. */
      iostream = NULL;
    }
  else
    {
      if (mbox)
	{
	  imap4d_enter_critical ();
	  mu_mailbox_flush (mbox, 0);
	  mu_mailbox_close (mbox);
	  manlock_unlock (mbox);
	  mu_mailbox_destroy (&mbox);
	  imap4d_leave_critical ();
	}

      switch (reason)
	{
	case ERR_NO_MEM:
	  io_untagged_response (RESP_BYE,
				"Server terminating: no more resources.");
	  mu_diag_output (MU_DIAG_ERROR, _("not enough memory"));
	  break;
	  
	case ERR_TERMINATE:
	  status = EX_OK;
	  io_untagged_response (RESP_BYE, "Server terminating on request.");
	  mu_diag_output (MU_DIAG_NOTICE, _("terminating on request"));
	  break;

	case ERR_SIGNAL:
	  mu_diag_output (MU_DIAG_ERROR, _("quitting on signal"));
	  exit (status);
	  
	case ERR_TIMEOUT:
	  status = EX_TEMPFAIL;
	  io_untagged_response (RESP_BYE, "Session timed out");
	  if (state == STATE_NONAUTH)
	    mu_diag_output (MU_DIAG_INFO, _("session timed out for no user"));
	  else
	    mu_diag_output (MU_DIAG_INFO, _("session timed out for user: %s"),
			    auth_data->name);
	  break;

	case ERR_NO_OFILE:
	  status = EX_IOERR;
	  mu_diag_output (MU_DIAG_INFO, _("write error on control stream"));
	  break;

	case ERR_NO_IFILE:
	  status = EX_IOERR;
	  mu_diag_output (MU_DIAG_INFO, _("read error on control stream"));
	  break;

	case ERR_MAILBOX_CORRUPTED:
	  status = EX_OSERR;
	  mu_diag_output (MU_DIAG_ERROR, _("mailbox modified by third party"));
	  break;
	  
	case ERR_STREAM_CREATE:
	  status = EX_UNAVAILABLE;
	  mu_diag_output (MU_DIAG_ERROR, _("cannot create transport stream"));
	  break;
      
	case OK:
	  status = EX_OK;
	  io_untagged_response (RESP_BYE, "Session terminating.");
	  if (state == STATE_NONAUTH)
	    mu_diag_output (MU_DIAG_INFO, _("session terminating"));
	  else
	    mu_diag_output (MU_DIAG_INFO,
			    _("session terminating for user: %s"),
			    auth_data->name);
	  break;

	default:
	  io_untagged_response (RESP_BYE, "Quitting (reason unknown)");
	  mu_diag_output (MU_DIAG_ERROR, _("quitting (numeric reason %d)"),
			  reason);
	  break;
	}
      
      if (status == EX_OK && command)
	io_completion_response (command, RESP_OK, "Completed");
    }
  
  util_bye ();

  closelog ();
  exit (status);
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012 Free Software Foundation, Inc.

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

int tls_available;

#ifdef WITH_TLS

static int tls_done;

/*
6.2.1.  STARTTLS Command

   Arguments:  none

   Responses:  no specific response for this command

   Result:     OK - starttls completed, begin TLS negotiation
               BAD - command unknown or arguments invalid
*/
int
imap4d_starttls (struct imap4d_session *session,
                 struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int status;

  if (!tls_available || tls_done)
    return io_completion_response (command, RESP_BAD, "Invalid command");

  if (imap4d_tokbuf_argc (tok) != 2)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");

  util_atexit (mu_deinit_tls_libs);

  status = io_completion_response (command, RESP_OK, "Begin TLS negotiation");
  io_flush ();

  if (imap4d_init_tls_server () == 0)
    tls_encryption_on (session);
  else
    {
      mu_diag_output (MU_DIAG_ERROR, _("session terminated"));
      util_bye ();
      exit (EX_OK);
    }

  return status;
}

void
tls_encryption_on (struct imap4d_session *session)
{
  tls_done = 1;
  imap4d_capability_remove (IMAP_CAPA_STARTTLS);
      
  login_disabled = 0;
  imap4d_capability_remove (IMAP_CAPA_LOGINDISABLED);

  session->tls_mode = tls_no;
  imap4d_capability_remove (IMAP_CAPA_XTLSREQUIRED);
}

void
starttls_init ()
{
  tls_available = mu_check_tls_environment ();
  if (tls_available)
    tls_available = mu_init_tls_libs (1);
  if (tls_available)
    imap4d_capability_add (IMAP_CAPA_STARTTLS);
}

#endif /* WITH_TLS */

/* EOF */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#include "imap4d.h"

#ifdef WITH_TLS

static int tls_available;
static int tls_done;

int
imap4d_starttls (struct imap4d_command *command, char *arg)
{
  int status;
  char *sp = NULL;

  if (!tls_available || tls_done)
    return util_finish (command, RESP_BAD, "Invalid command");

  if (util_getword (arg, &sp))
    return util_finish (command, RESP_BAD, "Too many args");

  status = util_finish (command, RESP_OK, "Begin TLS negotiation");
  util_flush_output ();
  tls_done = imap4d_init_tls_server ();

  if (tls_done)
    {
      imap4d_capability_remove ("STARTTLS");
      login_disabled = 0;
      imap4d_capability_remove ("LOGINDISABLED");
      util_atexit (mu_deinit_tls_libs);
    }
  return status;
}

void
starttls_init ()
{
  tls_available = mu_check_tls_environment ();
  if (tls_available)
    tls_available = mu_init_tls_libs ();
  if (tls_available)
    imap4d_capability_add ("STARTTLS");
}

#endif /* WITH_TLS */

/* EOF */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2003 Free Software Foundation, Inc.

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

char *capa[] = {
  "IMAP4rev1",
  "NAMESPACE",
  "X-VERSION",
  NULL
};

int
imap4d_capability (struct imap4d_command *command, char *arg)
{
  int i;

  (void) arg;
  util_send ("* CAPABILITY");

  for (i = 0; capa[i]; i++)
    util_send (" %s", capa[i]);

#ifdef WITH_TLS
  if (tls_available && !tls_done)
    util_send (" STARTTLS");
#endif /* WITH_TLS */

  imap4d_auth_capability ();
  util_send ("\r\n");

  return util_finish (command, RESP_OK, "Completed");
}

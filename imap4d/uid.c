/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

/*
 * see comment in store.c, raise to the nth power
 * this is really going to sux0r (maybe)
 */

int
imap4d_uid (struct imap4d_command *command, char *arg)
{
  char *cmd;
  char *sp = NULL;
  int rc = RESP_NO;
  char buffer[64];

  cmd = util_getword (arg, &sp);
  if (!cmd)
    util_finish (command, RESP_BAD, "Too few args");
  if (strcasecmp (cmd, "FETCH") == 0)
    {
      rc = imap4d_fetch0 (sp, 1, buffer, sizeof buffer);
    }
  else if (strcasecmp (cmd, "COPY") == 0)
    {
      rc = imap4d_copy0 (sp, 1, buffer, sizeof buffer);
    }
  else if (strcasecmp (cmd, "STORE") == 0)
    {
      rc = imap4d_store0 (sp, 1, buffer, sizeof buffer);
    }
  else if (strcasecmp (cmd, "SEARCH") == 0)
    {
      rc = imap4d_search0 (sp, 1, buffer, sizeof buffer);
    }
  else
    {
      snprintf (buffer, sizeof buffer, "Error uknown uid command");
      rc = RESP_BAD;
    }
  return util_finish (command, rc, "%s %s", cmd, buffer);
}

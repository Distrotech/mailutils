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

static list_t capa_list;

void
imap4d_capability_add (const char *str)
{
  if (!capa_list)
    list_create (&capa_list);
  list_append (capa_list, (void*)str);
}

void
imap4d_capability_remove (const char *str)
{
  list_remove (capa_list, (void*)str);
}

void
imap4d_capability_init ()
{
  static char *capa[] = {
    "IMAP4rev1",
    "NAMESPACE",
    "X-VERSION",
    NULL
  };
  int i;

  for (i = 0; capa[i]; i++)
    imap4d_capability_add (capa[i]);
}

static int
print_capa (void *item, void *data)
{
  util_send (" %s", (char *)item);
  return 0;
}

int
imap4d_capability (struct imap4d_command *command, char *arg)
{
  int i;

  (void) arg;
  util_send ("* CAPABILITY");

  list_do (capa_list, print_capa, NULL);
  
  imap4d_auth_capability ();
  util_send ("\r\n");

  return util_finish (command, RESP_OK, "Completed");
}

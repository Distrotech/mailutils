/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"

char *capa[] = {
  "IMAP4rev1",
  "NAMESPACE",
#ifdef WITH_GSSAPI
  "AUTH=GSSAPI",
#endif
  "X-VERSION",
  NULL
};

int
imap4d_capability (struct imap4d_command *command, char *arg)
{
  int i;
  
  (void)arg;
  util_send ("* CAPABILITY");
  for (i = 0; capa[i]; i++)
    util_send(" %s", capa[i]);
  util_send("\r\n");

  return util_finish (command, RESP_OK, "Completed");
}

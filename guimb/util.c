/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include "guimb.h"

void
util_error (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "guimb: ");
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}

char *
util_get_sender (int msgno)
{
  header_t header = NULL;
  address_t addr = NULL;
  message_t msg = NULL;
  char buffer[512];

  mailbox_get_message (mbox, msgno, &msg);
  message_get_header (msg, &header);
  if (header_get_value (header, MU_HEADER_FROM, buffer, sizeof (buffer), NULL)
      || address_create (&addr, buffer))
    {
      envelope_t env = NULL;
      message_get_envelope (msg, &env);
      if (envelope_sender (env, buffer, sizeof (buffer), NULL)
	  || address_create (&addr, buffer))
	{
	  util_error (_("can't determine sender name (msg %d)"), msgno);
	  return NULL;
	}
    }

  if (address_get_email (addr, 1, buffer, sizeof (buffer), NULL))
    {
      util_error (_("can't determine sender name (msg %d)"), msgno);
      address_destroy (&addr);
      return NULL;
    }

  address_destroy (&addr);
  return strdup (buffer);
}


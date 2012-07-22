/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2009-2012 Free Software
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

int
pop3d_uidl (char *arg, struct pop3d_session *sess)
{
  size_t mesgno;
  char uidl[128];
  mu_message_t msg;
  mu_attribute_t attr;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr (arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  if (strlen (arg) == 0)
    {
      size_t total = 0;
      pop3d_outf ("+OK\n");
      mu_mailbox_messages_count (mbox, &total);
      for (mesgno = 1; mesgno <= total; mesgno++)
        {
          mu_mailbox_get_message (mbox, mesgno, &msg);
          mu_message_get_attribute (msg, &attr);
          if (!pop3d_is_deleted (attr))
            {
              mu_message_get_uidl (msg, uidl, sizeof (uidl), NULL);
              pop3d_outf ("%s %s\n", mu_umaxtostr (0, mesgno), uidl);
            }
        }
      pop3d_outf (".\n");
    }
  else
    {
      mesgno = strtoul (arg, NULL, 10);
      if (mu_mailbox_get_message (mbox, mesgno, &msg) != 0)
        return ERR_NO_MESG;
      mu_message_get_attribute (msg, &attr);
      if (pop3d_is_deleted (attr))
        return ERR_MESG_DELE;
      mu_message_get_uidl (msg, uidl, sizeof (uidl), NULL);
      pop3d_outf ("+OK %s %s\n", mu_umaxtostr (0, mesgno), uidl);
    }

  return OK;
}

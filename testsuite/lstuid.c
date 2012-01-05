/* lstuid.c: List UIDs in mailbox
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/mailutils.h>

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox;
  size_t i, count;
  mu_message_t msg;
  
  if (argc != 2)
    {
      fprintf (stderr, "usage: %s MBOX\n", argv[0]);
      return 1;
    }

  mu_registrar_record (mu_mbox_record);

  /* Open the mailbox */
  MU_ASSERT (mu_mailbox_create (&mbox, argv[1]));
  MU_ASSERT (mu_mailbox_open (mbox, MU_STREAM_RDWR));
  mu_mailbox_messages_count (mbox, &count);
  for (i = 1; i <= count; i++)
    {
      size_t uid;
      MU_ASSERT (mu_mailbox_get_message (mbox, i, &msg));
      MU_ASSERT (mu_message_get_uid (msg, &uid));
      printf ("%lu: %lu\n", (unsigned long) i, (unsigned long) uid);
    }
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return 0;
}

/* ufms.c - a test of "UNIX mailbox first message symptom"
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

/* UFMS, or "UNIX mailbox first message symptom", is a bug that 
   existed in all versions of Mailutils up to 2.2.1 and 2.9.90
   as of 2010-11-07.

   Due to improperly designed expunge routine, the body of the very
   first message in a UNIX mailbox would get corrupted under certain
   circumstances.  The following three lines would appear at the top
   of the body:

   -IMAPbase: X Y
   Status: S
   X-UID: Z

   where (X, Y, Z are decimal numbers and S is a status string).

   Prerequisites for triggering the bug:

   1. Mailbox contains more than one message.
   2. The first message does not carry X-IMAPbase and X-UID headers.

   Recipe for triggering the bug:

   1. Delete some messages.
   2. Expunge the mailbox.
   3. Modify attributes of the first message.
   4. Flush your changes.
*/   
   

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox;
  size_t i, count;
  mu_message_t msg;
  mu_attribute_t attr;
  
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
  if (count < 2)
    {
      fprintf (stderr,
	       "%s: input mailbox should contain at least two messages\n",
	       argv[0]);
      exit (1);
    }
  /* Delete all records except the first and expunge the mailbox. */
  for (i = 2; i <= count; i++)
    {
      MU_ASSERT (mu_mailbox_get_message (mbox, i, &msg));
      MU_ASSERT (mu_message_get_attribute (msg, &attr));
      MU_ASSERT (mu_attribute_set_deleted (attr));
    }
  mu_mailbox_expunge (mbox);

  /* Modify attributes on the remaining message and synch */
  MU_ASSERT (mu_mailbox_get_message (mbox, 1, &msg));
  MU_ASSERT (mu_message_get_attribute (msg, &attr));
  MU_ASSERT (mu_attribute_set_read (attr));
  mu_mailbox_sync (mbox);

  /* Cleanup & exit */
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return 0;
}

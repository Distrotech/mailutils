/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* sfrom, Simple From */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/mailutils.h>

int
main (int argc, const char **argv)
{
  char *from;
  char *subject;
  mailbox_t mbox;
  int status;
  size_t msgno, total = 0;

  /* Register the type of mailbox. IMAP4, POP3 and local format  */
  {
    list_t registrar;
    registrar_get_list (&registrar);
    list_append (registrar, imap_record);
    list_append (registrar, path_record);
    list_append (registrar, pop_record);
  }

  status = mailbox_create_default (&mbox, argv[1]);

  if (status != 0)
    {
      fprintf (stderr, "mailbox_create: %s\n", mu_errstring (status));
      exit (EXIT_FAILURE);
    }

  status = mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      fprintf (stderr, "mailbox_open: %s\n", mu_errstring (status));
      exit (EXIT_FAILURE);
    }

  mailbox_messages_count (mbox, &total);

  for (msgno = 1; msgno <= total; msgno++)
    {
      message_t msg;
      header_t hdr;

      if ((status = mailbox_get_message (mbox, msgno, &msg)) != 0
          || (status = message_get_header (msg, &hdr)) != 0)
        {
          fprintf (stderr, "Error message: %s\n",
                   mu_errstring (status));
          exit (EXIT_FAILURE);
        }

      if (header_aget_value (hdr, MU_HEADER_FROM, &from))
        from = strdup ("(NO FROM)");

      if (header_aget_value (hdr, MU_HEADER_SUBJECT, &subject))
        subject = strdup("(NO SUBJECT)");

      printf ("%s\t%s\n", from, subject);
      free (from);
      free (subject);
    }

  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

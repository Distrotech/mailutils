/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mailutils/mailutils.h>

int
main (int argc, const char **argv)
{
  mu_mailbox_t mbox;
  size_t msgno, total = 0;
  int status;

  /* Register the formats. */
  mu_register_all_mbox_formats ();

  status = mu_mailbox_create_default (&mbox, argv[1]);
  if (status != 0)
    {
      mu_error ("mu_mailbox_create: %s", mu_strerror (status));
      exit (EXIT_FAILURE);
    }

  status = mu_mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      mu_error ("mu_mailbox_open: %s", mu_strerror (status));
      exit (EXIT_FAILURE);
    }

  status = mu_mailbox_messages_count (mbox, &total);
  if (status != 0)
    {
      mu_error ("mu_mailbox_messages_count: %s", mu_strerror (status));
      exit (EXIT_FAILURE);
    }

  for (msgno = 1; msgno <= total; msgno++)
    {
      mu_message_t msg;
      mu_header_t hdr;
      const char *from;
      const char *subject;

      if ((status = mu_mailbox_get_message (mbox, msgno, &msg)) != 0
          || (status = mu_message_get_header (msg, &hdr)) != 0)
        {
          mu_error ("Error message: %s", mu_strerror (status));
          exit (EXIT_FAILURE);
        }

      if (mu_header_sget_value (hdr, MU_HEADER_FROM, &from))
        from = "(NO FROM)";

      if (mu_header_sget_value (hdr, MU_HEADER_SUBJECT, &subject))
        subject = "(NO SUBJECT)";

      printf ("%s\t%s\n", from, subject);
    }

  status = mu_mailbox_close (mbox);
  if (status != 0)
    {
      mu_error ("mu_mailbox_close: %s", mu_strerror (status));
      exit (EXIT_FAILURE);
    }

  mu_mailbox_destroy (&mbox);
  return 0;
}

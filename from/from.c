/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

 /**
  *
  * Created as an example for using mailutils API
  * Sean 'Shaleh' Perry <shaleh@debian.org>, 1999
  * Alain Magloire alainm@gnu.org
  *
  **/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>

int
main(int argc, char **argv)
{
  mailbox_t mbox;
  size_t i;
  size_t count = 0;
  char *mailbox_name = NULL;
  /* Arbitrary limits.  A better approach would be to allocate
     as we go along but it is not worth the trouble.  */
  char buf[128];
  char personal[128];
  int status;

  /* have an argument */
  if (argc > 1)
    mailbox_name = argv[1];

  /* Register the desire formats.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
    list_append (bookie, imap_record);
  }

  if ((status = mailbox_create_default (&mbox, mailbox_name)) != 0)
    {
      fprintf (stderr, "could not create/open: %s\n", mu_errstring (status));
      exit (1);
    }

  /* Debuging Trace.  */
  {
    //mu_debug_t debug;
    //mailbox_get_debug (mbox, &debug);
    //mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
  }

  if ((status = mailbox_open (mbox, MU_STREAM_READ)) != 0)
    {
      fprintf (stderr, "could not create/open: %s\n", mu_errstring (status));
      exit (1);
    }

  mailbox_messages_count (mbox, &count);
  for (i = 1; i <= count; ++i)
    {
      message_t msg;
      header_t hdr;
      size_t len = 0;
      if ((status = mailbox_get_message (mbox, i, &msg)) != 0
	  || (status = message_get_header (msg, &hdr)) != 0)
	{
	  fprintf (stderr, "msg %d : %s\n", i, mu_errstring(status));
	  exit(2);
	}

      header_get_value (hdr, MU_HEADER_FROM, buf, sizeof (buf), &len);
      if (len != 0)
	{
	  address_t address = NULL;
	  address_create (&address, buf);
	  len = 0;
	  address_get_personal (address, 1, personal, sizeof (personal), &len);
	  printf ("%s\t", (len != 0) ? personal : buf);
	  address_destroy (&address);
	}
      else
	{
	  header_get_value (hdr, MU_HEADER_TO, buf, sizeof (buf), &len);
	  printf ("%s\t", buf);
	}

      header_get_value (hdr, MU_HEADER_SUBJECT, buf, sizeof (buf), NULL);
      printf ("%s\n", buf);
    }
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

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

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#include <mailutils/mailbox.h>

#ifndef _PATH_MAILDIR
# define _PATH_MAILDIR "/var/spool/mail"
#endif

#ifndef VERSION
# define VERSION "unknow"
#endif

int
main(int argc, char **argv)
{
  mailbox_t mbox;
  size_t i;
  size_t count = 0;
  char *mailbox_name = NULL;
  char from[BUFSIZ];
  char subject[BUFSIZ];
  int status;

  /* have an argument */
  if (argc > 1)
    mailbox_name = argv[1];

  if ((status = mailbox_create_default (&mbox, mailbox_name)) != 0
      || (status = mailbox_open (mbox, MU_MAILBOX_RDONLY)) != 0)
    {
      fprintf (stderr, "could not create/open: %s\n", strerror (status));
      exit (1);
    }

  mailbox_messages_count (mbox, &count);
  for(i = 1; i <= count; ++i)
    {
      message_t msg;
      header_t hdr;
      if ((status = mailbox_get_message (mbox, i, &msg)) != 0
	  || (status = message_get_header (msg, &hdr)) != 0)
	{
	  fprintf (stderr, "msg %d : %s\n", i, strerror(status));
	  exit(2);
	}
      header_get_value (hdr, MU_HEADER_FROM, from, 30, NULL);
      header_get_value (hdr, MU_HEADER_SUBJECT, subject, 40, NULL);

      fprintf(stdout, "%s\t%s\n", from, subject);

    }
  mailbox_close(mbox);
  return 0;
}

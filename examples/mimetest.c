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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>

#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/header.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>

void message_display_parts(message_t msg, const char *indent);

char from[256];
char subject[256];

int
main (int argc, char **argv)
{
  mailbox_t mbox = NULL;
  int ret;
  size_t i;
  size_t count = 0;
  char *mailbox_name = argv[1];
  int debug = 0;

  if (strcmp("-d", mailbox_name) == 0)
    {
      debug = 1;
      mailbox_name = argv[2];
    }

  /* Registration.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, file_record);
    list_append (bookie, imap_record);
    list_append (bookie, mbox_record);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
  }

  if ((ret = mailbox_create_default (&mbox, mailbox_name)) != 0)
    {
      fprintf (stderr, "could not create - %s\n", mu_errstring (ret));
      exit (2);
    }

  /* Debugging trace. */
  if (debug)
    {
      mu_debug_t debug;
      mailbox_get_debug (mbox, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);
    }

  /* Open the mailbox for reading only.  */
  if ((ret = mailbox_open (mbox, MU_STREAM_RDWR)) != 0)
    {
      fprintf (stderr, "mailbox open - %s\n", mu_errstring (ret));
      exit (2);
    }

  /* Iterate through the entire message set.  */
  mailbox_messages_count (mbox, &count);

  for (i = 1; i <= count; ++i)
    {
      message_t msg;
      header_t hdr;
      size_t nparts;
      size_t msize;

      if ((ret = mailbox_get_message (mbox, i, &msg)) != 0)
	{
	  fprintf (stderr, "mailbox_get_message - %s\n", mu_errstring (ret));
	  exit (2);
	}
      if ((ret = message_size (msg, &msize)) != 0)
	{
	  fprintf (stderr, "message_size - %s\n", mu_errstring (ret));
	  exit (2);
	}
      if ((ret = message_get_header (msg, &hdr)) != 0)
	{
	  fprintf (stderr, "message_get_header - %s\n", mu_errstring (ret));
	  exit (2);
	}
      header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
      header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
			NULL);
      printf ("-- Message: %d\n", i);
      printf ("-- From: %s\n", from);
      printf ("-- Subject: %s\n", subject);

      if ((ret = message_get_num_parts (msg, &nparts)) != 0)
	{
	  fprintf (stderr, "message_get_num_parts - %s\n", mu_errstring (ret));
	  exit (2);
	}
      printf ("-- Number of parts in message - %d\n", nparts);
      printf ("-- Total message size - %d\n", msize);
      message_display_parts (msg, "  ");
    }
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

char buf[2048];

void
message_display_parts (message_t msg, const char *indent)
{
  int ret, j;
  size_t msize, nparts, nsubparts;
  message_t part;
  header_t hdr;
  char type[256];
  char encoding[256];
  stream_t str;
  body_t body;
  int offset, ismulti;
  size_t nbytes;

  /* How many parts does the message has? */
  if ((ret = message_get_num_parts (msg, &nparts)) != 0)
    {
      fprintf (stderr, "message_get_num_parts - %s\n", mu_errstring (ret));
      exit (2);
    }

  /* Iterate through all the parts.
     Treat type "message/rfc822" differently, since it is a message of
     its own that can have other subparts(recursive). */
  for (j = 1; j <= nparts; j++)
    {
      if ((ret = message_get_part (msg, j, &part)) != 0)
	{
	  fprintf (stderr, "mime_get_part - %s\n", mu_errstring (ret));
	  exit (2);
	}
      if ((ret = message_size (part, &msize)) != 0)
	{
	  fprintf (stderr, "message_size - %s\n", mu_errstring (ret));
	  exit (2);
	}
      if ((ret = message_get_header (part, &hdr)) != 0)
	{
	  fprintf (stderr, "message_get_header - %s\n", mu_errstring (ret));
	  exit (2);
	}
      header_get_value (hdr, MU_HEADER_CONTENT_TYPE, type, sizeof (type),
			NULL);
      printf ("%sType of part %d = %s\n", indent, j, type);
      printf ("%sMessage part size - %d\n", indent, msize);
      encoding[0] = '\0';
      header_get_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, encoding,
			sizeof (encoding), NULL);
      ismulti = 0;
      if ((type[0]
	   && strncasecmp (type, "message/rfc822", strlen (type)) == 0)
	  || (message_is_multipart (part, &ismulti) == 0 && ismulti))
	{
	  char tmp[10];

	  if (!ismulti)
	    {
	      ret = message_unencapsulate (part, &part, NULL);
	      if (ret != 0)
		fprintf (stderr, "message_unencapsulate - %s\n",
			 mu_errstring (ret));
	      break;
	    }
	  if ((ret = message_get_header (part, &hdr)) != 0)
	    {
	      fprintf (stderr, "message_get_header - %s\n", mu_errstring (ret));
	      exit (2);
	    }
	  header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
	  header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
			    NULL);
	  printf ("%sEncapsulated message : %s\t%s\n", indent, from, subject);
	  printf
	    ("%s-------------------------------------------------------------------\n",
	     indent);
	  if ((ret = message_get_num_parts (part, &nsubparts)) != 0)
	    {
	      fprintf (stderr, "mime_get_num_parts - %s\n", mu_errstring (ret));
	      exit (2);
	    }
	  strcpy (tmp, indent);
	  strcat (tmp, "\t");
	  message_display_parts (part, tmp);
	  message_destroy (&part, NULL);
	}
      else if (type[0] == '\0'
	       || (strncasecmp (type, "text/plain", strlen ("text/plain")) ==
		   0)
	       || (strncasecmp (type, "text/html", strlen ("text/html")) ==
		   0))
	{
	  printf ("%sText Message\n", indent);
	  printf
	    ("%s-------------------------------------------------------------------\n",
	     indent);
	  message_get_body (part, &body);
	  body_get_stream (body, &str);
	  filter_create (&str, str, encoding, 0, 0);
	  offset = 0;
	  while (stream_readline (str, buf, sizeof (buf), offset, &nbytes) ==
		 0 && nbytes)
	    {
	      printf ("%s%s", indent, buf);
	      offset += nbytes;
	    }
	  stream_destroy (&str, NULL);
	}
      else
	{
	  /* Save the attachements.  */
	  char *fname = NULL;
	  message_aget_attachment_name (part, &fname);
	  if (fname == NULL)
	    {
	      char buffer[PATH_MAX + 1];
	      fname = tempnam (getcwd (buffer, PATH_MAX), "msg-");
	    }
	  printf ("%sAttachment - saving [%s]\n", indent, fname);
	  printf
	    ("%s-------------------------------------------------------------------\n",
	     indent);
	  /*FIXME: What is the 'data' argument for? */
	  message_save_attachment (part, NULL, NULL);
	  free (fname);
	}
      printf
	("\n%s End -------------------------------------------------------------------\n",
	 indent);
    }
}


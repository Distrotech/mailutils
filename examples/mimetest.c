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

#include <mailutils/mailutils.h>

void message_display_parts(message_t msg, int indent);

char from[256];
char subject[256];
int print_attachments;
int indent_level = 4;

void
print_file (const char *fname, int indent)
{
  char buf[128];
  FILE *fp = fopen (fname, "r");

  if (!fp)
    {
      fprintf (stderr, "can't open file %s: %s", fname, strerror (errno));
      return;
    }

  while (fgets (buf, sizeof buf, fp))
    printf ("%*.*s%s", indent, indent, "", buf);
  fclose (fp);
  unlink (fname);
}

int
main (int argc, char **argv)
{
  mailbox_t mbox = NULL;
  int ret;
  size_t i;
  size_t count = 0;
  char *mailbox_name;
  int debug = 0;

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-d") == 0)
	debug = 1;
      else if (strcmp (argv[i], "-p") == 0)
	print_attachments = 1;
      else if (strcmp (argv[i], "-i") == 0)
	indent_level = strtoul (argv[++i], NULL, 0);
      else
	break;
    }

  mailbox_name = argv[i];

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
      printf ("Message: %d\n", i);
      printf ("From: %s\n", from);
      printf ("Subject: %s\n", subject);

      if ((ret = message_get_num_parts (msg, &nparts)) != 0)
	{
	  fprintf (stderr, "message_get_num_parts - %s\n", mu_errstring (ret));
	  exit (2);
	}
      printf ("Number of parts in message - %d\n", nparts);
      printf ("Total message size - %d\n", msize);
      message_display_parts (msg, 0);
    }
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

char buf[2048];

void
message_display_parts (message_t msg, int indent)
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
      printf ("%*.*sType of part %d = %s\n", indent, indent, "", j, type);
      printf ("%*.*sMessage part size - %d\n", indent, indent, "", msize);
      encoding[0] = '\0';
      header_get_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, encoding,
			sizeof (encoding), NULL);
      ismulti = 0;
      if ((type[0]
	   && strncasecmp (type, "message/rfc822", strlen (type)) == 0)
	  || (message_is_multipart (part, &ismulti) == 0 && ismulti))
	{
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
	      fprintf (stderr, "message_get_header - %s\n",
		       mu_errstring (ret));
	      exit (2);
	    }
	  header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
	  header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
			    NULL);
	  printf ("%*.*sEncapsulated message : %s\t%s\n",
		  indent, indent, "", from, subject);
	  printf ("%*.*sBegin\n", indent, indent, "");
	  if ((ret = message_get_num_parts (part, &nsubparts)) != 0)
	    {
	      fprintf (stderr, "mime_get_num_parts - %s\n",
		       mu_errstring (ret));
	      exit (2);
	    }
	  message_display_parts (part, indent+indent_level);
	  message_destroy (&part, NULL);
	}
      else if (type[0] == '\0'
	       || (strncasecmp (type, "text/plain", strlen ("text/plain")) ==
		   0)
	       || (strncasecmp (type, "text/html", strlen ("text/html")) ==
		   0))
	{
	  printf ("%*.*sText Message\n", indent, indent, "");
	  printf ("%*.*sBegin\n", indent, indent, "");
	  message_get_body (part, &body);
	  body_get_stream (body, &str);
	  filter_create (&str, str, encoding, 0, 0);
	  offset = 0;
	  while (stream_readline (str, buf, sizeof (buf), offset, &nbytes) ==
		 0 && nbytes)
	    {
	      printf ("%*.*s%s", indent, indent, "", buf);
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
	    fname = mu_tempname (NULL);

	  printf ("%*.*sAttachment - saving [%s]\n", indent, indent, "",
		  fname);
	  printf ("%*.*sBegin\n", indent, indent, "");
	  /*FIXME: What is the 'data' argument for? */
	  message_save_attachment (part, NULL, NULL);
	  if (print_attachments)
	    print_file (fname, indent);
	  free (fname);
	}
      printf ("\n%*.*sEnd\n", indent, indent, "");
    }
}


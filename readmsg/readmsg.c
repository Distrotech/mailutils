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

#include "readmsg.h"

mailbox_t mbox;
size_t total;

static char from[256];
static char subject[256];

static void display_parts (message_t, const char *indent);

/* This is still work in progress  */
int
main(int argc, char **argv)
{
  int status;
  int *set = NULL;
  int n = 0;
  size_t i;
  char *mailbox_name = NULL;

  /* FIXME: Parse options:  See readmsg(1) part of elm:
     readmsg 1 3 0
     extracts three messages from the folder: the first, the third, and
     the last. */

  /* Check this in the option --folder.  */

  /* Registration.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
    list_append (bookie, imap_record);
  }

  status = mailbox_create_default (&mbox, mailbox_name);
  if (status != 0)
    {
      fprintf (stderr, "could not create - %s\n", strerror(status));
      exit (2);
    }

  /* Debuging Trace.  */
  if ( 0 )
    {
      debug_t debug;
      mailbox_get_debug (mbox, &debug);
      debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
    }

  status = mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      fprintf (stderr, "could not open - %s\n", strerror(status));
      exit (2);
    }

  mailbox_messages_count (mbox, &total);

  /* Build an array containing the message number.  */
  if (argc > 1)
    msgset (argc - 1, &argv[1], &set, &n);
  else
    {
      const char *av[] = { "*" };
      msgset (1, av, &set, &n);
    }

  for (i = 0; i < n; ++i)
    {
      message_t msg;
      header_t hdr;
      size_t msize;

      status = mailbox_get_message (mbox, set[i], &msg);
      if (status != 0)
	{
	  fprintf (stderr, "mailbox_get_message - %s\n", strerror (status));
	  exit (2);
	}

      status = message_get_header (msg, &hdr);
      if (status != 0)
	{
	  fprintf (stderr, "message_get_header - %s\n", strerror(status));
	  exit(2);
	}

      header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
      header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
			NULL);
      fprintf(stdout, "From: %s\tSubject: %s\n", from, subject);

      status = message_size (msg, &msize);
      if (status != 0)
	{
	  fprintf (stderr, "message_size - %s\n", strerror(status));
	  exit(2);
	}
      fprintf (stdout, "-- Total message size - %d\n", msize);

      display_parts (msg, "\t");
    }
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

static char buf[2048];

static void
display_parts (message_t message, const char *indent)
{
  int status, j;
  size_t msize, nparts;
  message_t msg;
  header_t hdr;
  char type[256];
  char encoding[256];
  int is_multi = 0;
  char *nl;

  status = message_get_num_parts (message, &nparts);
  if (status != 0)
    {
      fprintf (stderr, "message_get_num_parts - %s\n", strerror (status));
      exit (2);
    }
  fprintf(stdout, "%s-- Number of parts in message - %d\n", indent, nparts);

  for (j = 1; j <= nparts; j++)
    {
      status = message_get_part (message, j, &msg);
      if (status != 0 )
	{
	  fprintf (stderr, "message_get_part - %s\n", strerror (status));
	  exit (2);
	}

      status = message_get_header (msg, &hdr);
      if (status != 0)
	{
	  fprintf (stderr, "message_get_header - %s\n", strerror (status));
	  exit (2);
	}

      type[0] = '\0';
      header_get_value (hdr, MU_HEADER_CONTENT_TYPE, type, sizeof (type),
			NULL);

      nl = strchr (type, '\n');
      while ((nl = strchr (type, '\n')) != NULL)
	{
	  *nl = ' ';
	}
      fprintf(stdout, "%sType of part %d = %s\n", indent, j, type);

      status = message_size (msg, &msize);
      if (status != 0)
	{
	  fprintf (stderr, "message_size - %s\n", strerror (status));
	  exit (2);
	}
      fprintf(stdout, "%sMessage part size - %d\n",indent, msize);

      encoding[0] = '\0';
      header_get_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, encoding,
			sizeof (encoding), NULL);

      if (type[0] && strncasecmp (type, "message/rfc822", strlen (type)) == 0)
	{
	  message_t submsg = NULL;
	  char tmp[10];
	  tmp[0] = '\0';

	  status = message_unencapsulate (msg, &submsg, NULL);
	  if (status != 0)
	    {
	      fprintf (stderr, "message_unencapsulate - %s\n", strerror (status));
	      exit (2);
	    }

	  status = message_get_header (submsg, &hdr);
	  if (status != 0)
	    {
	      fprintf (stderr, "message_get_header - %s\n", strerror (status));
	      exit (2);
	    }

	  header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
	  header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
			    NULL);
	  fprintf (stdout, "%sEncapsulated message : %s\t%s\n", indent, from,
		   subject);
	  fprintf (stdout, "%s----------------------------------------------\
---------------------\n", indent);

	  strcpy (tmp, indent);
	  strcat (tmp,"\t");
	  display_parts (submsg, tmp);
	  message_destroy (&submsg, NULL);
	}
      else if ((strncasecmp (type, "text/plain", strlen ("text/plain")) == 0)
	       || (strncasecmp (type, "text/html", strlen ("text/html")) == 0))
	{
	  stream_t d_stream = NULL;
	  stream_t b_stream = NULL;
	  body_t body;
	  size_t nbytes = 0;
	  int offset = 0;
	  fprintf (stdout, "%sText Message\n",indent);
	  fprintf (stdout, "%s-------------------------------------------------------------------\n", indent);
	  message_get_body (msg, &body);
	  body_get_stream (body, &b_stream);
	  //d_stream = b_stream;
	  //status = decoder_stream_create(&d_stream, b_stream, encoding);
	  status = filter_create (&d_stream, b_stream, encoding, 0, 0);
	  stream_setbufsiz (d_stream, 128);
	  if (status != 0)
	    {
	      d_stream = b_stream;
	    }
	  while (stream_readline (d_stream, buf, sizeof (buf),
				  offset, &nbytes ) == 0 && nbytes )
	    {
	      fprintf (stdout, "%s%s", indent, buf);
	      offset += nbytes;
	    }
	  if (status == 0)
	    stream_destroy(&d_stream, NULL);
	}
    else
      {
	message_is_multipart (msg, &is_multi);
	if (is_multi)
	  {
	    char tmp[24];
	    memset (tmp, '\0', sizeof (tmp));
	    strcpy(tmp, indent);
	    strcat(tmp,"\t");
	    display_parts (msg, tmp);
	  }
	else
	  {
	    body_t body = NULL;
	    stream_t stream = NULL;
	    size_t nbytes = 0;
	    int offset = 0;
	    message_get_body (msg, &body);
	    body_get_stream (body, &stream);
#if 0
	    while (stream_readline (stream, buf, sizeof (buf),
				    offset, &nbytes ) == 0 && nbytes )
	      {
		fprintf (stdout, "%s%s", indent, buf);
		offset += nbytes;
	      }
#endif
	  }
#if 0
	{
	  char *fname;
	  message_attachment_filename ( msg, &fname);
	  if ( fname == NULL )
	    {
	      char buffer[PATH_MAX+1];
	      fname = tempnam (getcwd(buffer, PATH_MAX), "msg-" );
	    }
	  fprintf (stdout, "%sAttachment - saving [%s]\n",indent, fname);
	  fprintf (stdout, "%s-------------------------------------------------------------------\n", indent);
	  message_save_attachment (msg, fname, NULL);
	  free (fname);
	}
#endif

      }
      fprintf(stdout, "\n%s End -------------------------------------------------------------------\n", indent);
    }
}

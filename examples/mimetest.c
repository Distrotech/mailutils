/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>

#include <mailutils/mailutils.h>

void message_display_parts(mu_message_t msg, int indent);

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

#define CATCH(expr)                                                           \
 do                                                                           \
   {                                                                          \
     int rc;                                                                  \
     if ((rc = expr) != 0)                                                    \
       {                                                                      \
         fprintf (stderr, "%s: %s\n", #expr, mu_strerror (rc));               \
         exit (2);                                                            \
       }                                                                      \
   }                                                                          \
 while (0)

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox = NULL;
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
  mu_registrar_record (mu_imap_record);
  mu_registrar_record (mu_mbox_record);
  mu_registrar_record (mu_path_record);
  mu_registrar_record (mu_pop_record);

  CATCH (mu_mailbox_create_default (&mbox, mailbox_name));
  
  /* Debugging trace. */
  if (debug)
    {
      mu_debug_t debug;
      mu_mailbox_get_debug (mbox, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);
    }

  /* Open the mailbox for reading only.  */
  CATCH (mu_mailbox_open (mbox, MU_STREAM_RDWR));

  /* Iterate through the entire message set.  */
  mu_mailbox_messages_count (mbox, &count);

  for (i = 1; i <= count; ++i)
    {
      mu_message_t msg;
      mu_header_t hdr;
      size_t nparts;
      size_t msize, nlines;

      CATCH (mu_mailbox_get_message (mbox, i, &msg));
      CATCH (mu_message_size (msg, &msize));
      CATCH (mu_message_lines (msg, &nlines));
      CATCH (mu_message_get_header (msg, &hdr));
      mu_header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
      mu_header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
                        NULL);
      printf ("Message: %lu\n", (unsigned long) i);
      printf ("From: %s\n", from);
      printf ("Subject: %s\n", subject);

      CATCH (mu_message_get_num_parts (msg, &nparts));
      printf ("Number of parts in message - %lu\n",
	      (unsigned long) nparts);
      printf ("Total message size - %lu/%lu\n",
	      (unsigned long) msize, (unsigned long) nlines);
      message_display_parts (msg, 0);
    }
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return 0;
}

char buf[2048];

static void
print_message_part_sizes (mu_message_t part, int indent)
{
  mu_body_t body;
  mu_header_t hdr;
  size_t msize, mlines, hsize, hlines, bsize, blines;
  
  CATCH (mu_message_size (part, &msize));
  CATCH (mu_message_lines (part, &mlines));
  CATCH (mu_message_get_header (part, &hdr));
  CATCH (mu_header_size (hdr, &hsize));
  CATCH (mu_header_lines (hdr, &hlines));
  CATCH (mu_message_get_body (part, &body));
  CATCH (mu_body_size (body, &bsize));
  CATCH (mu_body_lines (body, &blines));
  printf ("%*.*sMessage part size - %lu/%lu: %lu/%lu, %lu/%lu\n",
	  indent, indent, "",
	  (unsigned long) msize, (unsigned long) mlines,
	  (unsigned long) hsize, (unsigned long) hlines,
	  (unsigned long) bsize, (unsigned long) blines);
}
  
void
message_display_parts (mu_message_t msg, int indent)
{
  int ret, j;
  size_t nparts, nsubparts;
  mu_message_t part;
  mu_header_t hdr;
  char type[256];
  char encoding[256];
  mu_stream_t str;
  mu_body_t body;
  int offset, ismulti;
  size_t nbytes;

  /* How many parts does the message has? */
  if ((ret = mu_message_get_num_parts (msg, &nparts)) != 0)
    {
      fprintf (stderr, "mu_message_get_num_parts - %s\n", mu_strerror (ret));
      exit (2);
    }

  /* Iterate through all the parts.
     Treat type "message/rfc822" differently, since it is a message of
     its own that can have other subparts(recursive). */
  for (j = 1; j <= nparts; j++)
    {
      CATCH (mu_message_get_part (msg, j, &part));
      CATCH (mu_message_get_header (part, &hdr));
      mu_header_get_value (hdr, MU_HEADER_CONTENT_TYPE, type, sizeof (type),
                        NULL);
      printf ("%*.*sType of part %d = %s\n", indent, indent, "", j, type);
      print_message_part_sizes (part, indent);
      encoding[0] = '\0';
      mu_header_get_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, encoding,
                        sizeof (encoding), NULL);
      ismulti = 0;
      if ((type[0]
           && strncasecmp (type, "message/rfc822", strlen (type)) == 0)
          || (mu_message_is_multipart (part, &ismulti) == 0 && ismulti))
        {
          if (!ismulti)
	    CATCH (mu_message_unencapsulate (part, &part, NULL));
	  
          CATCH (mu_message_get_header (part, &hdr));
          mu_header_get_value (hdr, MU_HEADER_FROM, from, sizeof (from), NULL);
          mu_header_get_value (hdr, MU_HEADER_SUBJECT, subject, sizeof (subject),
                            NULL);
          printf ("%*.*sEncapsulated message : %s\t%s\n",
                  indent, indent, "", from, subject);
          printf ("%*.*sBegin\n", indent, indent, "");
          CATCH (mu_message_get_num_parts (part, &nsubparts));
          message_display_parts (part, indent+indent_level);
          mu_message_destroy (&part, NULL);
        }
      else if (type[0] == '\0'
               || (strncasecmp (type, "text/plain", strlen ("text/plain")) ==
                   0)
               || (strncasecmp (type, "text/html", strlen ("text/html")) ==
                   0))
        {
          printf ("%*.*sText Message\n", indent, indent, "");
          printf ("%*.*sBegin\n", indent, indent, "");
          mu_message_get_body (part, &body);
          mu_body_get_stream (body, &str);
          mu_filter_create (&str, str, encoding, 0, 0);
          offset = 0;
          while (mu_stream_readline (str, buf, sizeof (buf), offset, &nbytes) ==
                 0 && nbytes)
            {
              printf ("%*.*s%s", indent, indent, "", buf);
              offset += nbytes;
            }
          mu_stream_destroy (&str, NULL);
        }
      else
        {
          /* Save the attachements.  */
          char *fname = NULL;
          mu_message_aget_attachment_name (part, &fname);
          if (fname == NULL)
            fname = mu_tempname (NULL);

          printf ("%*.*sAttachment - saving [%s]\n", indent, indent, "",
                  fname);
          printf ("%*.*sBegin\n", indent, indent, "");
          /*FIXME: What is the 'data' argument for? */
          mu_message_save_attachment (part, NULL, NULL);
          if (print_attachments)
            print_file (fname, indent);
          free (fname);
        }
      printf ("\n%*.*sEnd\n", indent, indent, "");
    }
}


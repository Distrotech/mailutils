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

#include "mail.h"

/*
  FIXME:
 decode, this is temporary, until the API on how to present
 mime/attachements etc is less confusing.
 */

struct decode_closure
{
  int select_hdr;
};

static int print_stream __P ((stream_t, FILE *));
static int display_message __P ((message_t, msgset_t *msgset,
				 struct decode_closure *closure));
static int display_message0 __P ((FILE *, message_t, const msgset_t *, int));
static int mailcap_lookup __P ((const char *));
static int get_content_encoding __P ((header_t hdr, char **value));

int
mail_decode (int argc, char **argv)
{
  msgset_t *msgset;
  struct decode_closure decode_closure;
  
  if (msgset_parse (argc, argv, &msgset))
    return 1;

  decode_closure.select_hdr = islower (argv[0][0]);

  util_msgset_iterate (msgset, display_message, &decode_closure);

  msgset_free (msgset);
  return 0;
}

int
display_message (message_t mesg, msgset_t *msgset,
		 struct decode_closure *closure)
{
  FILE *out;
  size_t lines = 0;
  
  if (util_isdeleted (msgset->msg_part[0]))
    return 1;
  
  message_lines (mesg, &lines);
  if ((util_find_env("crt"))->set && lines > util_getlines ())
    out = popen (getenv("PAGER"), "w");
  else
    out = ofile;

  display_message0 (out, mesg, msgset, closure->select_hdr);

  if (out != ofile)
    pclose (out);

  /* Mark enclosing message as read */
  if (mailbox_get_message (mbox, msgset->msg_part[0], &mesg) == 0)
    {
      attribute_t attr;
      message_get_attribute (mesg, &attr);
      attribute_set_read (attr);
    }
  return 0;
}
  
static void
display_headers (FILE *out, message_t mesg, const msgset_t *msgset,
		 int select_hdr)
{
  header_t hdr = NULL;
  /* Print the selected headers only.  */
  if (select_hdr)
    {
      size_t num = 0;
      size_t i = 0;
      char buffer[512];
      
      message_get_header (mesg, &hdr);
      header_get_field_count (hdr, &num);
      for (i = 1; i <= num; i++)
	{
	  buffer[0] = '\0';
	  header_get_field_name (hdr, i, buffer, sizeof(buffer), NULL);
	  if (mail_header_is_visible (buffer))
	    {
	      fprintf (out, "%s: ", buffer);
	      header_get_field_value (hdr, i, buffer, sizeof(buffer), NULL);
	      fprintf (out, "%s\n", buffer);
	    }
	}
      fprintf (out, "\n");
    }
  else /* Print displays all headers.  */
    {
      stream_t stream = NULL;
      if (message_get_header (mesg, &hdr) == 0
	  && header_get_stream (hdr, &stream) == 0)
	print_stream (stream, out);
    }
}

void
display_part_header (FILE *out, const msgset_t *msgset,
		     char *type, char *encoding)
{
  int size = util_screen_columns () - 3;
  int i;

  fputc ('+', out);
  for (i = 0; i <= size; i++)
    fputc ('-', out);
  fputc ('+', out);
  fputc ('\n', out);
  fprintf (out, "| Message=%d", msgset->msg_part[0]);
  for (i = 1; i < msgset->npart; i++)
    fprintf (out, "[%d", msgset->msg_part[i]);
  for (i = 1; i < msgset->npart; i++)
    fprintf (out, "]");
  fprintf (out, "\n");
	      
  fprintf (out, "| Type=%s\n", type);
  fprintf (out, "| encoding=%s\n", encoding);
  fputc ('+', out);
  for (i = 0; i <= size; i++)
    fputc ('-', out);
  fputc ('+', out);
  fputc ('\n', out);
}

static int
display_message0 (FILE *out, message_t mesg, const msgset_t *msgset,
		  int select_hdr)
{
  size_t nparts = 0;
  header_t hdr = NULL;
  char *type;
  char *encoding;
  int ismime = 0;
  
  message_get_header (mesg, &hdr);
  util_get_content_type (hdr, &type);
  get_content_encoding (hdr, &encoding);

  message_is_multipart (mesg, &ismime);
  if (ismime)
    {
      int j;

      message_get_num_parts (mesg, &nparts);

      for (j = 1; j <= nparts; j++)
	{
	  message_t message = NULL;

	  if (message_get_part (mesg, j, &message) == 0)
	    {
	      msgset_t *set = msgset_expand (msgset_dup (msgset),
					     msgset_make_1 (j));
	      display_message0 (out, message, set, 0);
	      msgset_free (set);
	    }
	}
    }
  else if (strncasecmp (type, "message/rfc822", strlen (type)) == 0)
    {
      message_t submsg = NULL;
      
      if (message_unencapsulate (mesg, &submsg, NULL) == 0)
	display_message0 (out, submsg, msgset, select_hdr);
    }
  else if (mailcap_lookup (type))
    {
      /* FIXME: lookup .mailcap and do the appropriate action when
	 an match engry is find.  */
      /* Do something, spawn a process etc ....  */
    }
  else /*if (strncasecmp (type, "text/plain", strlen ("text/plain")) == 0
	 || strncasecmp (type, "text/html", strlen ("text/html")) == 0)*/
    {
      body_t body = NULL;
      stream_t b_stream = NULL;
      
      display_part_header (out, msgset, type, encoding);
      display_headers (out, mesg, msgset, select_hdr);

      if (message_get_body (mesg, &body) == 0 &&
	  body_get_stream (body, &b_stream) == 0)
	{
	  stream_t d_stream = NULL;
	  stream_t stream = NULL;
	  
	  /* Can we decode.  */
	  if (filter_create(&d_stream, b_stream, encoding,
			    MU_FILTER_DECODE, MU_STREAM_READ) == 0)
	    stream = d_stream;
	  else
	    stream = b_stream;
	  
	  print_stream (stream, out);
	  if (d_stream)
	    stream_destroy (&d_stream, NULL);
	}
    }
  
  free (type);
  free (encoding);

  return 0;
}

static int
print_stream (stream_t stream, FILE *out)
{
  char buffer[512];
  off_t off = 0;
  size_t n = 0;

  while (stream_read (stream, buffer, sizeof (buffer) - 1, off, &n) == 0
	 && n != 0)
    {
      if (ml_got_interrupt())
	{
	  util_error("\nInterrupt");
	  break;
	}
      buffer[n] = '\0';
      fprintf (out, "%s", buffer);
      off += n;
    }
  return 0;
}

static int
get_content_encoding (header_t hdr, char **value)
{
  char *encoding = NULL;
  util_get_hdr_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, &encoding);
  if (encoding == NULL || *encoding == '\0')
    {
      if (encoding)
	free (encoding);
      encoding = strdup ("7bit"); /* Default.  */
    }
  *value = encoding;
  return 0;
}


static int
mailcap_lookup (const char *type)
{
  (void)type;
  return 0;
}

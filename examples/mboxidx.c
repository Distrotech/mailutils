/* Convert mailbox to a Sphinx XML input.
   Copyright (C) 2014-2015 Free Software Foundation, Inc.

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

/* This program takes a mailbox in arbitrary format as its only argument
   and produces on standard output an XML stream suitable for use with
   Sphinx for indexing the mailbox content.  Example usage in sphinx.conf:

   source mbox_source
   {
        type = xmlpipe2
        xmlpipe_command = /usr/local/bin/mboxidx /var/spool/archive/mbox
   }

   index mbox_idx
   {
        source = mbox_source
	docinfo = extern
        charset_type = utf-8
	...
   }
*/

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <mailutils/mailutils.h>
#include <fnmatch.h>

mu_list_t typelist;
mu_list_t fldlist;
int log_to_stderr = 1;
char *output_charset = "utf-8";
int no_out_if_empty = 0;

char *docstr[] = {
  "usage: mboxidx [OPTIONS] MBOX",
  "converts MBOX into a xmlpipe2 stream for Sphinx indexing engine",
  "OPTIONS are:",
  "",
  "    -c CHARSET         set output charset",
  "    -d SPEC            set debug verbosity",
  "    -f FIELD           index header FIELD",
  "    -F FACILITY        set syslog facility",
  "    -L TAG             set syslog tag",
  "    -N                 no output if nothing to index",
  "    -S N               skip first N messages",
  "    -s FILE            save and read status of the prior run from FILE",
  "    -t GLOB            index message body if its content type matches GLOB",
  "    -h                 display this help summary",
  NULL
};

void
help (int c)
{
  int i;
  FILE *fp = c ? stderr : stdout;

  for (i = 0; docstr[i]; i++)
    fprintf (fp, "%s\n", docstr[i]);

  exit (c);
}


static int
matchstr (void const *item, void const *data)
{
  char const *pat = item;
  char const *str = data;

  return fnmatch (pat, str, 0);
}

struct field
{
  char *name;
  int ishdr;
};

static void
fldadd (char *name, int ishdr)
{
  struct field *f = mu_alloc (sizeof (*f));
  f->name = mu_strdup (name);
  f->ishdr = ishdr;
  mu_list_append (fldlist, f);
}

static int
get_hdr_value (mu_header_t hdr, const char *name, char **value)
{
  int status = mu_header_aget_value_unfold (hdr, name, value);
  if (status == 0)
    mu_rtrim_class (*value, MU_CTYPE_SPACE);
  return status;
}


static int
get_content_encoding (mu_header_t hdr, char **value)
{
  char *encoding = NULL;
  if (get_hdr_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING, &encoding))
    encoding = mu_strdup ("7bit");	/* Default.  */
  *value = encoding;
  return 0;
}

static int
set_charset_filter (mu_stream_t * str, char *input_charset)
{
  int rc;
  char const *args[4];

  mu_stream_t b_stream = *str;

  args[0] = "iconv";
  args[1] = input_charset;
  args[2] = output_charset;
  args[3] = "copy-octal";
  args[4] = NULL;

  rc = mu_filter_create_args (str, b_stream,
			      "iconv",
			      4, args, MU_FILTER_DECODE, MU_STREAM_READ);

  if (rc == 0)
    mu_stream_unref (b_stream);
  return rc;
}


static void
formatbody (mu_message_t msg, int delim)
{
  int rc;
  char *type;
  char *input_charset = NULL;
  int ismime = 0;
  mu_message_t msgpart;
  mu_header_t hdr = NULL;

  mu_message_get_header (msg, &hdr);
  if (get_hdr_value (hdr, MU_HEADER_CONTENT_TYPE, &type))
    type = mu_strdup ("text/plain");
  else
    {
      struct mu_wordsplit ws;

      mu_strlower (type);

      ws.ws_delim = ";";
      if (mu_wordsplit (type, &ws,
			MU_WRDSF_DELIM | MU_WRDSF_WS
			| MU_WRDSF_NOVAR | MU_WRDSF_NOCMD))
	{
	  mu_error ("can't split content type \"%s\": %s",
		    type, mu_wordsplit_strerror (&ws));
	}
      else
	{
	  int i;
	  
	  type = ws.ws_wordv[0];
	  ws.ws_wordv[0] = NULL;
	  for (i = 1; i < ws.ws_wordc; i++)
	    {
	      if (strncasecmp (ws.ws_wordv[i], "charset=", 8) == 0)
		{
		  input_charset = mu_strdup (ws.ws_wordv[i] + 8);
		  break;
		}
	    }
	  mu_wordsplit_free (&ws);
	}
    }

  mu_message_is_multipart (msg, &ismime);
  if (ismime)
    {
      size_t i, nparts;

      rc = mu_message_get_num_parts (msg, &nparts);
      if (rc)
	mu_diag_funcall (MU_DIAG_ERROR, "mu_message_get_num_parts", NULL, rc);
      else
	for (i = 1; i <= nparts; i++)
	  {
	    if (mu_message_get_part (msg, i, &msgpart) == 0)
	      formatbody (msgpart, 1);
	  }
    }
  else if (mu_c_strncasecmp (type, "message/rfc822", strlen (type)) == 0)
    {
      rc = mu_message_unencapsulate (msg, &msgpart, NULL);
      if (rc)
	mu_diag_funcall (MU_DIAG_ERROR, "mu_message_unencapsulate", NULL, rc);
      else
	formatbody (msgpart, 1);
    }
  else if (mu_list_locate (typelist, type, NULL) == 0)
    {
      char *encoding;
      mu_body_t body = NULL;
      mu_stream_t b_stream = NULL;
      mu_stream_t d_stream = NULL;
      mu_stream_t stream = NULL;

      get_content_encoding (hdr, &encoding);

      mu_message_get_body (msg, &body);
      mu_body_get_streamref (body, &b_stream);
      if (mu_filter_create (&d_stream, b_stream, encoding,
			    MU_FILTER_DECODE, MU_STREAM_READ) == 0)
	{
	  mu_stream_unref (b_stream);
	  stream = d_stream;
	}
      else
	stream = b_stream;

      if (!input_charset || set_charset_filter (&stream, input_charset))
	set_charset_filter (&stream, "US-ASCII");

      b_stream = stream;
      if (mu_filter_create (&stream, b_stream, "xml",
			    MU_FILTER_ENCODE, MU_STREAM_READ) == 0)
	mu_stream_unref (b_stream);

      if (delim)
	mu_printf ("\n");
      mu_stream_copy (mu_strout, stream, 0, NULL);

      mu_stream_unref (stream);
      free (encoding);
    }
  free (type);
}

static int
fldfmt (void *item, void *data)
{
  struct field *f = item;
  mu_message_t msg = data;
  int rc;

  mu_printf ("    <%s>", f->name);
  if (f->ishdr)
    {
      mu_header_t hdr;
      char *hfield;

      mu_message_get_header (msg, &hdr);
      rc = mu_header_aget_value_unfold (hdr, f->name, &hfield);
      if (rc == 0)
	{
	  char *tmp;

	  rc = mu_rfc2047_decode (output_charset, hfield, &tmp);
	  if (rc)
	    mu_stream_write (mu_strout, hfield, strlen (hfield), NULL);
	  else
	    {
	      mu_stream_write (mu_strout, tmp, strlen (tmp), NULL);
	      free (tmp);
	    }
	}
    }
  else
    formatbody (msg, 0);

  mu_printf ("</%s>\n", f->name);
  return 0;
}

static int
fldout (void *item, void *data)
{
  struct field *f = item;
  mu_printf ("    <sphinx:field name=\"%s\"/>\n", f->name);
  return 0;
}

void
xmlpipe2_header ()
{
  mu_printf ("<?xml version=\"1.0\"?>\n");
  mu_printf ("<sphinx:docset>\n");
  mu_printf ("  <sphinx:schema>\n");
  mu_list_foreach (fldlist, fldout, NULL);
  mu_printf ("  </sphinx:schema>\n");
}

void
xmlpipe2_footer ()
{
  mu_printf ("</sphinx:docset>\n");
}

size_t skip_count;
size_t nmesg;

void
read_state_file (const char *name)
{
  FILE *fp;
  size_t count;
  int c;

  if (access (name, F_OK))
    {
      if (errno == ENOENT)
	return;
      mu_error ("can't access state file \"%s\": %s",
		name, mu_strerror (errno));
      exit (1);
    }

  fp = fopen (name, "r");
  if (!fp)
    {
      mu_error ("can't open state file \"%s\" for reading: %s",
		name, mu_strerror (errno));
      exit (1);
    }

  count = 0;
  while ((c = fgetc (fp)) != EOF && c >= '0' && c <= '9')
    {
      size_t n = count * 10 + c - '0';
      if (n < count)
	{
	  mu_error ("%s: message number too big", name);
	  exit (1);
	}
      count = n;
    }
  fclose (fp);

  if (c != EOF && c != '\n')
    {
      mu_error ("%s: malformed state file", name);
      exit (1);
    }

  skip_count = count;
}

void
write_state_file (const char *name)
{
  FILE *fp;

  fp = fopen (name, "w");
  if (!fp)
    {
      mu_error ("can't open state file \"%s\" for writing: %s",
		name, mu_strerror (errno));
      exit (1);
    }

  fprintf (fp, "%lu\n", (unsigned long) nmesg);

  fclose (fp);
}

static int
action (mu_observer_t o, size_t type, void *data, void *action_data)
{
  mu_mailbox_t mbox = mu_observer_get_owner (o);
  mu_message_t msg = NULL;

  switch (type)
    {
    case MU_EVT_MESSAGE_ADD:
      ++nmesg;
      if (nmesg < skip_count)
	break;
      if (no_out_if_empty && nmesg == skip_count)
	xmlpipe2_header ();

      MU_ASSERT (mu_mailbox_get_message (mbox, nmesg, &msg));
      mu_printf ("  <sphinx:document id=\"%lu\">\n", (unsigned long) nmesg);
      mu_list_foreach (fldlist, fldfmt, msg);
      mu_printf ("  </sphinx:document>\n");
      break;
    case MU_EVT_MAILBOX_PROGRESS:
      /* Noop.  */
      break;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int rc;
  mu_mailbox_t mbox;
  mu_observer_t observer;
  mu_observable_t observable;
  size_t total;
  char *p;
  char *state_file = NULL;

  mu_set_program_name (argv[0]);
  mu_register_all_mbox_formats ();
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  MU_ASSERT (mu_list_create (&fldlist));

  MU_ASSERT (mu_list_create (&typelist));
  mu_list_set_comparator (typelist, matchstr);
  mu_list_append (typelist, mu_strdup ("text/*"));

  while ((rc = getopt (argc, argv, "c:d:f:F:hL:NS:s:t:")) != EOF)
    {
      switch (rc)
	{
	case 'c':
	  output_charset = optarg;
	  break;
	case 'd':
	  mu_debug_parse_spec (optarg);
	  break;
	case 'F':
	  if (mu_string_to_syslog_facility (optarg, &mu_log_facility))
	    {
	      mu_error ("unknown facility: %s", optarg);
	      exit (1);
	    }
	  log_to_stderr = 0;
	  break;
	case 'f':
	  fldadd (optarg, 1);
	  break;
	case 'L':
	  mu_log_tag = optarg;
	  break;
	case 'N':
	  no_out_if_empty = 1;
	  break;
	case 'S':
	  skip_count = strtoul (optarg, &p, 10);
	  if (*p)
	    {
	      mu_error ("invalid number: %s", optarg);
	      exit (1);
	    }
	  break;
	case 's':
	  state_file = optarg;
	  break;
	case 't':
	  p = mu_strdup (optarg);
	  mu_strlower (p);
	  mu_list_append (typelist, p);
	  break;
	case 'h':
	  help (0);
	default:
	  exit (1);
	}
    }
  fldadd ("content", 0);

  if (state_file)
    read_state_file (state_file);

  skip_count++;

  argc -= optind;
  argv += optind;

  if (argc != 1)
    help (1);

  if (log_to_stderr)
    mu_stdstream_strerr_setup (MU_STRERR_STDERR);
  else
    {
      if (!mu_log_tag)
	mu_log_tag = (char *) mu_program_name;
      mu_stdstream_strerr_setup (MU_STRERR_SYSLOG);
    }

  rc = mu_mailbox_create_default (&mbox, argv[0]);
  if (rc)
    {
      mu_error ("mu_mailbox_create: %s", mu_strerror (rc));
      exit (EXIT_FAILURE);
    }

  rc = mu_mailbox_open (mbox, MU_STREAM_READ);
  if (rc)
    {
      mu_error ("mu_mailbox_open: %s", mu_strerror (rc));
      exit (EXIT_FAILURE);
    }

  mu_observer_create (&observer, mbox);
  mu_observer_set_action (observer, action, mbox);
  mu_observer_set_action_data (observer, NULL, mbox);
  mu_mailbox_get_observable (mbox, &observable);
  mu_observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);

  if (!no_out_if_empty)
    xmlpipe2_header ();
  MU_ASSERT (mu_mailbox_scan (mbox, 1, &total));
  if (!no_out_if_empty || nmesg >= skip_count)
    xmlpipe2_footer ();
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);

  if (state_file)
    write_state_file (state_file);

  return EXIT_SUCCESS;
}

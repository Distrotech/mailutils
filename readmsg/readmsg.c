/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "readmsg.h"

#define WEEDLIST_SEPARATOR " :,"

static void print_header __P ((message_t, int no_header,
			       int all_header, const char *weedlst));
static void print_body __P ((message_t));
static int  string_starts_with __P ((const char * s1, const char *s2));

const char *program_version = "readmsg (" PACKAGE_STRING ")";
static char doc[] = N_("GNU readmsg -- print messages");
static error_t readmsg_parse_opt  __P((int key, char *arg,
				       struct argp_state *astate));

static struct argp_option options[] = 
{
  { "debug", 'd', 0, 0, N_("Display debugging information"), 1 },
  { "header", 'h', 0, 0, N_("Display entire header"), 1 },
  { "weedlist", 'w', N_("LIST"), 0,
    N_("List of header names separated by whitespace or commas"), 1 },
  { "folder", 'f', N_("FOLDER"), 0, N_("Folder to use"), 1 },
  { "no-header", 'n', 0, 0, N_("Exclude all headers"), 1 },
  { "form-feeds", 'p', 0, 0, N_("Output formfeeds between messages"), 1 },
  { "show-all-match", 'a', NULL, 0,
    N_("Print all messages matching pattern, not just the first"), 1 },
  {0, 0, 0, 0}
};

static struct argp argp = {
  options,
  readmsg_parse_opt,
  NULL,
  doc,
  NULL,
  NULL, NULL
};

static const char *readmsg_argp_capa[] = {
  "common",
  "mailbox",
#ifdef WITH_TLS
  "tls",
#endif
  NULL
};

int dbug = 0;
const char *mailbox_name = NULL;
const char *weedlist = NULL;
int no_header = 0;
int all_header = 0;
int form_feed = 0;
int show_all = 0;

static error_t
readmsg_parse_opt (int key, char *arg, struct argp_state *astate)
{
  switch (key)
    {
    case 'd':
      dbug++;
      break;

    case 'h':
      all_header = 1;
      break;

    case 'f':
      mailbox_name = arg;
      break;

    case 'w':
      weedlist = arg;
      break;

    case 'n':
      no_header = 1;
      break;

    case 'p':
      form_feed = 1;
      break;
	  
    case 'a':
      show_all = 1;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static int
string_starts_with (const char * s1, const char *s2)
{
  const unsigned char * p1 = (const unsigned char *) s1;
  const unsigned char * p2 = (const unsigned char *) s2;
  int n = 0;

  /* Sanity.  */
  if (s1 == NULL || s2 == NULL)
    return n;

  while (*p1 && *p2)
    {
      if ((n = toupper (*p1++) - toupper (*p2++)) != 0)
	break;
    }
  return (n == 0);
}

static void
print_header (message_t message, int no_header, int all_headers,
	      const char *weedlist)
{
  header_t header = NULL;

  if (no_header)
    return;

  message_get_header (message, &header);

  if (all_headers)
    {
      stream_t stream = NULL;
      off_t offset = 0;
      size_t len = 0;
      char buf[128];

      header_get_stream (header, &stream);
      while (stream_read (stream, buf, sizeof (buf) - 1, offset, &len) == 0
	     && len != 0)
	{
	  buf[len] ='\0';
	  printf ("%s", buf);
	  offset += len;
	}
    }
  else
    {
      size_t count = 0;
      const char *delim = WEEDLIST_SEPARATOR;
      size_t i;

      header_get_field_count (header, &count);

      for (i = 1; i <= count; i++)
	{
	  char *name = NULL;
	  char *value = NULL;
	  char *weedcopy = strdup (weedlist);
	  char *token, *s;

	  header_aget_field_name (header, i, &name);
	  header_aget_field_value (header, i, &value);
	  for (token = strtok_r (weedcopy, delim, &s); token;
	       token = strtok_r (NULL, delim, &s)) 
	    {
	      if (string_starts_with (name, token))
		{
		  /* Check if header_aget_value return an empty string.  */
		  if (value && *value)
		    printf ("%s: %s\n", name, value);
		}
	    }
	  free (value);
	  free (name);
	  free (weedcopy);
	}
      putchar ('\n');
    }
}

static void
print_body (message_t message)
{
  char buf[128];
  body_t body = NULL;
  stream_t stream = NULL;
  off_t offset = 0;
  size_t len = 0;
  message_get_body (message, &body);
  body_get_stream (body, &stream);

  while (stream_read (stream, buf, sizeof (buf) - 1, offset, &len) == 0
	 && len != 0)
    {
      buf[len] ='\0';
      printf ("%s", buf);
      offset += len;
    }
}

/* This is still work in progress  */
/* FIXME: Parse options:  See readmsg(1) part of elm:
   readmsg 1 3 0
   extracts three messages from the folder: the first, the third, and
   the last. */
int
main (int argc, char **argv)
{
  int status;
  int *set = NULL;
  int n = 0;
  int i;
  int index;
  mailbox_t mbox = NULL;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, readmsg_argp_capa, &index, NULL);

  /* register the formats.  */
  mu_register_all_mbox_formats ();
  mu_register_extra_formats ();

  status = mailbox_create_default (&mbox, mailbox_name);
  if (status != 0)
    {
      fprintf (stderr, _("Could not create mailbox <%s>: %s.\n"),
	       mailbox_name ? mailbox_name : _("default"),
	       mu_strerror(status));
      exit (2);
    }

  /* Debuging Trace.  */
  if (dbug)
    {
      mu_debug_t debug;
      mailbox_get_debug (mbox, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
    }

  status = mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      url_t url = NULL;

      mailbox_get_url (mbox, &url);
      fprintf (stderr, _("Could not open mailbox <%s>: %s.\n"),
	       url_to_string (url), mu_strerror(status));
      exit (2);
    }

  if (weedlist == NULL)
    weedlist = "Date To Cc Subject From Apparently-";

  /* Build an array containing the message number.  */
  argc -= index;
  if (argc > 0)
    msglist (mbox, show_all, argc, &argv[index], &set, &n);

  for (i = 0; i < n; ++i)
    {
      message_t msg = NULL;

      status = mailbox_get_message (mbox, set[i], &msg);
      if (status != 0)
	{
	  mu_error ("mailbox_get_message: %s",
		    mu_strerror (status));
	  exit (2);
	}

      print_header (msg, no_header, all_header, weedlist);
      print_body (msg);
      if (form_feed)
	putchar ('\f');
      else
	putchar ('\n');
    }

  putchar ('\n');

  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

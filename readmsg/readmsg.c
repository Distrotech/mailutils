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
#define WEEDLIST_SEPARATOR  " :,"

static void usage __P ((int, const char *));
static void print_header __P ((message_t, int no_header, int all_header, const char *weedlst));
static void print_body __P ((message_t));
static int  string_starts_with __P ((const char * s1, const char *s2));

const char *short_options = "adnhpf:w:";
static struct option long_options[] =
{
  {"debug", no_argument, 0, 'd'},
  {"header", no_argument, 0, 'h'},
  {"weedlist", required_argument, 0, 'w'},
  {"folder", no_argument, 0, 'f'},
  {"no-header", no_argument, 0, 'n'},
  {"form-feeds", no_argument, 0, 'p'},
  {"show-all-match", required_argument, 0, 'a'},
  {"help", no_argument, 0, '&'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

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
print_header (message_t message, int no_header, int all_headers, const char *weedlist)
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
      while (stream_read (stream, buf, sizeof (buf) - 1, offset, &len) == 0 && len != 0)
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

      for (i = 1; i < count; i++)
	{
	  char *name = NULL;
	  char *value = NULL;
	  char *token = strdup (weedlist);
	  char *p = token;

	  header_aget_field_name (header, i, &name);
	  header_aget_field_value (header, i, &value);
	  for (; (token = strtok (token, delim)) != NULL; token = NULL)
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
	  free (p);
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
  while (stream_read (stream, buf, sizeof (buf) - 1, offset, &len) == 0 && len != 0)
    {
      buf[len] ='\0';
      printf ("%s", buf);
      offset += len;
    }
}

static void
usage (int status, const char *prognam)
{
  if (status == 0)
    {
      printf ("GNU Mailutils.\n");
      printf ("Usage: %s [OPTIONS]\n\n", prognam);
      printf ("  -d, --debug              display debuging information\n");
      printf ("  -h, --header             display the entire header\n");
      printf ("  -f, --folder=FILE        folder to use\n");
      printf ("  -w, --weelist=LIST       list of header names separated by whitespace or commas\n");
      printf ("  -n, --no-header          exclude all headers\n");
      printf ("  -p, --form-feeds         put form-feeds between messages instead of newline\n");
      printf ("  -a, --show-all-match     print all message matching PATTERN\n");
      printf ("      --help               display this help and exit\n");
      printf ("  -v, --version            display version information and exit\n");
      printf ("\nReport bugs to bug-mailutils@gnu.org\n");
    }
  else
    {
      printf ("Try: %s --help\n", prognam);
    }
  exit (status);
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
  int c;
  int dbug = 0;
  const char *mailbox_name = NULL;
  const char *weedlist = NULL;
  int no_header = 0;
  int all_header = 0;
  int form_feed = 0;
  int show_all = 0;
  mailbox_t mbox = NULL;

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (c)
        {
        case 'd':
          dbug++;
          break;

        case 'h':
          all_header = 1;
          break;

        case 'f':
          mailbox_name = optarg;
          break;

        case 'w':
          weedlist = optarg;
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

        case '&':
          usage (0, argv[0]);
          break;

        case 'v':
          printf ("Mailutils 0.0.0: readmsg\n");
          exit (0);
          break;

        default:
          usage (1, argv[0]);
          break;
        }
    }


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
      fprintf (stderr, "could not create - %s\n", mu_errstring(status));
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
      fprintf (stderr, "mailbox open - %s\n", mu_errstring(status));
      exit (2);
    }

  if (weedlist == NULL)
    weedlist = "Date To Cc Subject From Apparently-";

  /* Build an array containing the message number.  */
  argc -= optind;
  if (argc > 0)
    msglist (mbox, show_all, argc, &argv[optind], &set, &n);

  for (i = 0; i < n; ++i)
    {
      message_t msg = NULL;

      status = mailbox_get_message (mbox, set[i], &msg);
      if (status != 0)
	{
	  fprintf (stderr, "mailbox_get_message - %s\n", mu_errstring (status));
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

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <mailutils/mailbox.h>

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifndef _PATH_MAILDIR
# define _PATH_MAILDIR "/var/spool/mail"
#endif

#ifndef VERSION
# define VERSION "0.0"
#endif

static struct option long_options[] =
{
  {"help", 0, NULL, 'h'},
  {"version", 0, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
  char bar[80];
  char buf[128];
  char foo[80];
  mailbox_t mbox;
  message_t msg;
  header_t hdr;
  body_t body;
  attribute_t attr;
  stream_t os;
  int c;
  char *mboxname = NULL;
  size_t count = 0;
  int status;
  int num = 0;
  size_t n;


  c = getopt_long (argc, argv, "hv", long_options, (int *)0);

  switch (c)
    {
    case 'v':
      printf("mail (GNU mailutils) %s\n", VERSION);
      exit (0);
      break;
    case 'h':
      printf("\
Usage: mail [OPTION]...\n\
Access the current users mailbox through a simple text mode interface\n\
\n\
  --version  Show version information\n\
  --help     Show this message\n\
\n\
Report bugs to <bug-mailutils@gnu.org>.\n");
      exit (0);
      break;
    default:
      break;
    }

  if (argc > 1)
    mboxname = argv[1];

  if ((status = mailbox_create_default (&mbox, mboxname)) != 0 ||
      (status = mailbox_open (mbox, MU_MAILBOX_RDWR)) != 0)
    {
      fprintf (stderr, "Ack, %s, reading %s\n", strerror (status), mboxname);
      exit (1);
    }

  mailbox_messages_count (mbox, &count);
  printf ("Experimental Mail. Type ? for help\n");
  printf ("Number of messages: %d\n", count);

  while (1)
    {
      printf ("& ");
      fflush (stdout);
      fgets (bar, 80, stdin);
      c = bar[0];
      if (c == 'd' || c == 'D' || c == 'b' || c == 'B' || c == 'h' ||
	  c == 'H' || c == 'r' || c == 'R' || c == 'f' || c == 'F' ||
	  c == 'T' || c == 't' || c == 'p' || c == 'P')
	{
	  if (sscanf (bar, "%80s %d\n", foo, &num) == 1)
	    {
	      printf ("# ");
	      fgets (bar, 80, stdin);
	      num = atoi (bar);
	    }
	}
      switch (c)
	{
	case 'q':
	case 'Q':
	  printf ("<<Quit>>\n");
	  mailbox_close (mbox);
	  return 0;
	  break;
	case 'f':
	case 'F':
	  printf ("<<From %d>>\n", num);
	  {
	    size_t i;
	    if (num == 0)
	      num = 1;
	    for (i = num; i <= count; i++)
	      {
		if ((status = mailbox_get_message (mbox, i, &msg)) != 0 ||
		    (status = message_get_header (msg, &hdr)) != 0 ||
		    (status = header_get_value (hdr, "From", buf,
						sizeof(buf), &n)) != 0)
		  {
		    fprintf (stderr, "msg %d: %s\n", i, strerror (status));
		    break;
		  }
		else
		  printf ("%d From: %s\n", i, buf);
		if ((i % 10) == 0)
		  {
		    char tmp[16];
		    *tmp = '\0';
		    printf ("\nHit (q|Q)uit to break\n");
		    fgets (tmp, sizeof(tmp), stdin);
		    if (*tmp == 'q' || *tmp == 'Q')
		      break;
		  }
	      }
	  }
	  break;
	case 'r':
	case 'R':
	  printf ("<<Reply %d>>\n", num);
	  break;
	case 'h':
	case 'H':
	  printf ("<<Header %d>>\n", num);
	  if ((status = mailbox_get_message (mbox, num, &msg)) != 0 ||
	      (status = message_get_header (msg, &hdr)) != 0 ||
	      (status = header_get_stream (hdr, &os)) != 0)
	    fprintf (stderr, "msg %d: %s\n", num, strerror (status));
	  else
	    {
	      off_t off = 0;
	      while (stream_read (os, buf, sizeof (buf) - 1,
				  off, &n) == 0 && n != 0)
		{
		  buf[n] = '\0';
		  printf (buf);
		  off += n;
		}
	    }
	  break;
	case 'b':
	case 'B':
	  printf ("<<Body %d>>\n", num);
	  if ((status = mailbox_get_message (mbox, num, &msg)) != 0 ||
	      (status = message_get_body (msg, &body)) != 0 ||
	      (status = body_get_stream (body, &os)) != 0)
	    fprintf (stderr, "msg %d: %s\n", num, strerror (status));
	  else
	    {
	      off_t off = 0;
	      while (stream_read (os, buf, sizeof (buf) - 1,
				  off, &n) == 0 && n != 0)
		{
		  buf[n] = '\0';
		  printf (buf);
		  off += n;
		}
	    }
	  break;
	case 'd':
	case 'D':
	  printf ("<<Delete %d>>\n", num);
	  if ((status = mailbox_get_message (mbox, num, &msg)) != 0 ||
	      (status = message_get_attribute (msg, &attr)) != 0)
	    fprintf (stderr, "msg %d: %s\n", num, strerror (status));
	  else
	    {
	      status = attribute_set_deleted (attr);
	      if (status != 0)
		fprintf (stderr, "Delete %d: %s\n", num, strerror (status));
	      else
		printf ("msg %d mark deleted\n", num);
	    }
	  break;
	case 'u':
	case 'U':
	  printf ("<<Undelete %d>>\n", num);
	  if ((status = mailbox_get_message (mbox, num, &msg)) != 0 ||
	      (status = message_get_attribute (msg, &attr)) != 0)
	    fprintf (stderr, "msg %d: %s\n", num, strerror (status));
	  else
	    {
	      status = attribute_unset_deleted (attr);
	      if (status != 0)
		fprintf (stderr, "Undelete %d: %s\n", num, strerror (status));
	      else
		printf ("msg %d unmark deleted\n", num);
	    }
	  break;
	case 'x':
	case 'X':
	  printf ("<<Expunge>>\n");
	  status = mailbox_expunge (mbox);
	  if (status != 0)
	    printf ("Expunging: %s\n", strerror (status));
	  mailbox_messages_count (mbox, &count);
	  printf ("Number of messages: %d\n", count);
	  break;
	case 'p':
	case 'P':
	  printf ("<<Print %d>>\n", num);
	  if ((status = mailbox_get_message (mbox, num, &msg)) != 0 ||
	      (status = message_get_stream (msg, &os)) != 0)
	    {
	      fprintf (stderr, "msg %d: %s\n", num, strerror (status));
	      break;
	    }
	  else
	    {
	      off_t off = 0;
	      while (stream_read (os, buf, sizeof (buf) - 1,
				  off, &n) == 0 && n != 0)
		{
		  buf[n] = '\0';
		  printf (buf);
		  off += n;
		}
	    }
	  break;
	case '?':
	  printf ("\
 (d|D)elete  num-msg\n\
 (b|B)ody    num-msg\n\
 (f|F)rom    num-msg\n\
 (h|H)eader  num-msg\n\
 (p|P)rint   num-msg\n\
 (q|Q)uit    num-msg\n\
 (r|R)eply   num-msg\n\
e(x|X)punge  num-msg\n");
	  break;
	default:
	  break;
	}
    }
}

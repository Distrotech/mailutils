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
  * Created as an example of using libmailbox
  * Sean 'Shaleh' Perry <shaleh@debian.org>, 1999
  * Alain Magloire alainm@gnu.org
  *
  **/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailbox.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifndef _PATH_MAILDIR
# define _PATH_MAILDIR "/var/spool/mail"
#endif

#ifndef VERSION
# define VERSION "unknown"
#endif

#include "getopt.h"

/* Short options. */
static char const short_options[] =
"H:s:m:hv";

static int opt_from;
static int opt_to;
static int opt_cc;
static int opt_date;
static int opt_subject;
static int opt_status_new;;
static int opt_status_read;;
static int opt_status_old;;
static int opt_size;
static int opt_number;
static int opt_mailbox;

/* long options equivalence */
static struct option long_options[] =
{
  {"date", no_argument, &opt_date, 1},
  {"from", no_argument, &opt_from, 1},
  {"to", no_argument, &opt_to, 1},
  {"cc", no_argument, &opt_cc, 1},
  {"subject", no_argument, &opt_subject, 1},
  {"header", required_argument, NULL, 'H'},
  {"status", required_argument, NULL, 's'},
  {"size", no_argument, &opt_size, 1},
  {"number", no_argument, &opt_number, 1},
  {"mailbox", required_argument, &opt_mailbox, 'm'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

/* program name */
static char *program;

static void
usage (int type)
{
  switch (type)
    {
    case 1:
      printf("%s (GNU mailutils), version %s\n", program, VERSION);
      break;
    case 2:
      /* Fall Through */
    default:
      printf ("Usage: %s [OPTION] [mailbox]\n", program);
      printf ("Retrieve information from a mailbox\n\
\n\
  --date             equivalent to --header=Date\n\
  --from             equivalent to --header=From\n\
  --to               equivalent to --header=To\n\
  --cc               equivalent to --header=Cc\n\
  --subject          equivalent to --header=Subject\n\
  --header=HEADER    Specify the header to display\n\
                     HEADER is 'Date', 'From', 'To', 'Cc', 'Subject'\n\
                     default is --header=From --header=Subject\n\
  --status=STATUS    Display only message with a given status\n\
                     STATUS is 'new', 'read', 'unread'\n\
                     defaut is --status=new --status=read --status=unread\n\
  --size             Display mailbox size\n\
  --number           Display total of number of messages\n\
  --mailbox=MAILBOX  Specify another mailbox\n\
                     MAILBOX is url(pop://pop.server/user),
                     a file /var/mail/user, or user.
  -v, --version      Print version information and exit\n\
  -h, --help         Show this message\n\
\n\
Mailbox :
$MAIL is check for the default mailbox path, if not set
the environ variables $USER or $LOGNAME are use in the default mail spool.\n\
\n\
Report bugs to <bug-mailutils@gnu.org>.\n");
    }
}

int
main(int argc, char *argv[])
{
  mailbox_t mbox;
  size_t rvalue, i;
  size_t count = 0, size;
  char *user = NULL;
  char *mailbox_name = NULL;
  int  opt;
  char buffer[BUFSIZ];

  /* set program name */
  program = argv[0];
  if (program && strrchr (program, '/'))
    {
      program = strrchr (program, '/') + 1;
    }

  while ((opt = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    {
      switch (opt)
	{
	case 'H':
	  if (strcasecmp (optarg, "From") == 0)
	    opt_from = 1;
	  else if (strcasecmp (optarg, "To") == 0)
	    opt_to = 1;
	  else if (strcasecmp (optarg, "Cc") == 0)
	    opt_cc = 1;
	  else if (strcasecmp (optarg, "Date") == 0)
	    opt_date = 1;
	  else if (strcasecmp (optarg, "Subject") == 0)
	    opt_subject = 1;
	  else
	    {
	      fprintf (stderr, "Unknown header\n");
	    }
	  break;
	case 's':
	  if (strcasecmp (optarg, "new") == 0)
	    opt_status_new = 1;
	  else if (strcasecmp (optarg, "read") == 0)
	    opt_status_read = 1;
	  else if (strcasecmp (optarg, "old") == 0)
	    opt_status_old = 1;
	  else
	    {
	      fprintf (stderr, "Unknown status\n");
	    }
	  break;
	case 'm':
	  mailbox_name = optarg;
	  break;
	case 'v':
	  usage (1);
	  exit (0);
	  break;
	case 'h':
	  usage (2);
	  exit (0);
	  break;
	default:
	  usage (2);
	  exit (1);
	  break;
	}
    }

  /* have an argument */
  if (optind > 0)
   {
      mailbox_name = argv[optind];
      /* is it a URL */
      if (strchr (mailbox_name, ':') == NULL)
	{
	  /* is it a filename */
	  if (mailbox_name[0] != '/')
	    {
	      user = mailbox_name; /* a user name */
	      mailbox_name = NULL;
	    }
	}
    }
  else if (getenv ("MAIL"))
    {
      mailbox_name = getenv ("MAIL");
    }
  else
    {
      user = (getenv ("LOGNAME")) ? getenv ("LOGNAME") : getenv ("USER");
      if (user == NULL)
	{
	  fprintf (stderr, "who am I?\n");
	  exit (1);
	}
    }
  if (user)
    {
      snprintf (buffer, sizeof(buffer), "%s/%s", _PATH_MAILDIR, user);
      mailbox_name = buffer;
    }
  if (mailbox_init (&mbox, mailbox_name, 0) != 0
      || mailbox_open (mbox, MU_MB_RDONLY) != 0)
    {
      fprintf (stderr, "could not open\n");
      exit (2);
    }
  mailbox_scan (mbox, &count);
  for(i = 0; i < count; ++i) {
    rvalue = mailbox_get_header (mbox, i, 0, buffer, sizeof (buffer), &size);
    if (rvalue != 0)
      {
	fprintf (stderr, "header %s\n", strerror (rvalue));
	exit(2);
      }
    printf("%s\n", buffer);
  }
  mailbox_close(mbox);
  mailbox_destroy(&mbox);
  exit(0);
}

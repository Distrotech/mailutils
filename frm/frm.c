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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "getopt.h"

#include <mailutils/mailbox.h>
#include <mailutils/header.h>
#include <mailutils/registrar.h>
#include <mailutils/observer.h>
#include <mailutils/address.h>

static int action (observer_t, size_t);

static struct option long_options[] =
{
  {"help", no_argument, 0, 'h'},
  {"field", required_argument, 0, 'f'},
  {"to", no_argument, 0, 'l'},
  {"number", no_argument, 0, 'n'},
  {"Quiet", no_argument, 0, 'Q'},
  {"query", no_argument, 0, 'q'},
  {"summary", no_argument, 0, 'S'},
  {"status", required_argument, 0, 's'},
  {"align", no_argument, 0, 't'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

const char *short_options ="hf:lnQqSs:tv";

static char* show_field;
static int show_to;
static int show_from = 1;
static int show_subject = 1;
static int show_number;
static int show_summary;
static int be_quiet;
static int align = 1;
static int show_query;
static int have_new_mail;

#define IS_READ 0x001
#define IS_OLD  0x010
#define IS_NEW  0x100
static int select_attribute;
static int selected;


/* Retrieve the Personal Name from the header To: or From:  */
static int
get_personal (header_t hdr, char *field, char *personal, size_t buflen)
{
  char hfield[512];
  int status;

  /* Empty string.  */
  *hfield = '\0';

  status = header_get_value (hdr, field, hfield, sizeof (hfield), NULL);
  if (status == 0)
    {
      address_t address = NULL;
      size_t len = 0;
      address_create (&address, hfield);
      address_get_personal (address, 1, personal, buflen, &len);
      address_destroy (&address);
      if (len == 0)
	strncpy (personal, hfield, buflen)[buflen - 1] = '\0';
    }
  return status;
}

/* Observable Action this is being call at every message discover.  */
/* FIXME: The format of the display is poorly done, please correct.  */
static int
action (observer_t o, size_t type)
{
  static int counter;

  switch (type)
    {
    case MU_EVT_MESSAGE_ADD:
      {
	mailbox_t mbox = observer_get_owner (o);
	message_t msg = NULL;
	header_t hdr = NULL;
	attribute_t attr = NULL;

	counter++;

	mailbox_get_message (mbox, counter, &msg);

	message_get_attribute (msg, &attr);
	message_get_header (msg, &hdr);

	if (select_attribute
	    && (select_attribute & IS_READ)
	    && (!attribute_is_read (attr)))
	  {
	    break;
	  }
	else if (select_attribute
		 && (select_attribute & IS_NEW)
		 && (!attribute_is_recent (attr)))
	  {
	    break;
	  }
	else if (select_attribute
		 && (select_attribute & IS_OLD)
		 && (!attribute_is_seen (attr)))
	  {
	    break;
	  }

	if (attribute_is_recent (attr))
	  have_new_mail = 1;

	if (select_attribute)
	  selected = 1;

	if (show_number)
	  printf ("%d: ", counter);

	if (show_field)
	  {
	    char hfield[256];
	    int status = header_get_value (hdr, show_field, hfield,
					   sizeof (hfield), NULL);
	    if (status == 0)
	      printf ("%s", hfield);
	  }

	if (show_to)
	  {
	    char hto[16];
	    int status = get_personal (hdr, MU_HEADER_TO, hto, sizeof (hto));
	    if (status == 0)
	      printf ("(%s) ", hto);
	    else
	      printf ("(------)");
	  }

	if (show_from)
	  {
	    char hfrom[32];
	    int status = get_personal (hdr, MU_HEADER_FROM, hfrom,
				       sizeof (hfrom));
	    if (status == 0)
	      printf ("%s\t", hfrom);
	    else
	      printf ("-----\t");
	  }

	if (show_subject)
	  {
	    char hsubject[64];
	    int status = header_get_value (hdr, MU_HEADER_SUBJECT, hsubject,
					   sizeof (hsubject), NULL);
	    if(status == 0)
	      printf("%s", hsubject);
	  }
	printf ("\n");
	break;
      }

    case MU_EVT_MAILBOX_PROGRESS:
      /* Noop.  */
      break;
    }
  return 0;
}

void
usage (const char *argv)
{
  printf ("GNU Mailutils.\n");
  printf ("Usage: %s [OPTIONS]\n\n", argv);
  printf ("  -h, --help               display this help and exit\n");
  printf ("  -f, --field=string       header field to display\n");
  printf ("  -l, --to                 include the To: information\n");
  printf ("  -n, --number             display the message numbered\n");
  printf ("  -Q, --Quiet              very quiet\n");
  printf ("  -q, --query              print a message if unread mail\n");
  printf ("  -S, --summary            print a summary of messages\n");
  printf ("  -s, --status=[nor]       select message with the specific \
 attribute\n");
  printf ("                           [n]ew, [r]ead, [u]nread.\n");
  printf ("  -t, --align              Try to align\n");
  printf ("  -v, --version            display version information and exit\n");
  printf ("\nReport bugs to bug-mailutils@gnu.org\n");
  exit (3);
}

/* This is a clone of the elm program call "frm".  It is a good example on
   how to use the observable(callback) of libmailutils.  "frm" has to
   be very interactive, it is not possible to call mailbox_messages_count()
   and wait for the scanning to finish before displaying.  As soon as the scan
   find a new message we want to know about it, this is done by registering
   an observable type MU_MAILBOX_MSG_ADD.  The rest is formating code.  */

int
main(int argc, char **argv)
{
  char *mailbox_name = NULL;
  size_t total = 0;
  int c;
  int status = 0;

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 'h':
	  usage (argv[0]);
	  break;

	case 'f':
	  show_field = optarg;
	  show_from = 0;
	  show_subject = 0;
	  align = 0;
	  break;

	case 'l':
	  show_to = 1;
	  break;

	case 'n':
	  show_number = 1;
	  break;

	case 'Q':
	  /* Very silent.  */
	  be_quiet += 2;
	  if (freopen("/dev/null", "w", stdout) == NULL)
	    {
	      perror ("Can not be very quiet");
	      exit (3);
	    }
	  break;

	case 'q':
	  be_quiet = show_query = 1;
	  break;

	case 'S':
	  show_summary = 1;
	  break;

	case 's':
	  if (optarg)
	    switch (*optarg)
	      {
	      case 'r':
		select_attribute |= IS_READ;
		break;

	      case 'o':
		select_attribute |= IS_OLD;
		break;

	      case 'n':
		select_attribute |= IS_NEW;
		break;

	      }
	  break;

	case 't':
	  align = 1;
	  break;

	case 'v':
	  printf ("Mailutils 0.0.0: frm\n");
	  exit (3);
	  break;

	default:
	  break;
	}
    }

  /* have an argument */
  if (optind < argc)
    mailbox_name = argv[optind];

  /* register the formats.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
    list_append (bookie, imap_record);
  }

  /* Construct the mailbox_t, attach a notification and destroy  */
  {
    mailbox_t mbox;
    observer_t observer;
    observable_t observable;
    int status;

    if ((status = mailbox_create_default (&mbox, mailbox_name) != 0)
	|| (status = mailbox_open (mbox, MU_STREAM_READ) != 0))
      {
	fprintf (stderr, "could not open mailbox\n");
	exit (3);
      }

    if (! be_quiet)
      {
	observer_create (&observer, mbox);
	observer_set_action (observer, action, mbox);
	mailbox_get_observable (mbox, &observable);
	observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);
      }

    mailbox_scan (mbox, 1, &total);

    if (! be_quiet)
      {
	observable_detach (observable, observer);
	observer_destroy (&observer, mbox);
      }

    mailbox_close(mbox);
    mailbox_destroy(&mbox);
  }

  if (show_summary)
    printf ("You have %d messages\n", total);
  if (show_query && have_new_mail)
    printf ("You have new mail\n");

  /* 0 - selected messages discover.
     1 - have messages.
     2 - no message.
  */
  if (selected)
    status = 0;
  else if (total > 0)
    status = 1;
  else
    status = 2;
  return status;
}

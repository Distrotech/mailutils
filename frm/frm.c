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

#include <mu_argp.h>

#include <mailutils/address.h>
#include <mailutils/attribute.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>

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
static int dbug;

#define IS_READ 0x001
#define IS_OLD  0x010
#define IS_NEW  0x100
static int select_attribute;
static int selected;

static int action (observer_t, size_t);

const char *argp_program_version = "frm (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU frm -- display From: lines";

static struct argp_option options[] = {
  {"debug",  'd', NULL,   0, "Enable debugging output", 0},
  {"field",  'f', "NAME", 0, "Header field to display", 0},
  {"to",     'l', NULL,   0, "Include the To: information", 0},
  {"number", 'n', NULL,   0, "Display message numbers", 0},
  {"Quiet",  'Q', NULL,   0, "Very quiet", 0},
  {"query",  'q', NULL,   0, "Print a message if unread mail", 0},
  {"summary",'S', NULL,   0, "Print a summary of messages", 0},
  {"status", 's', "[nor]",0,
   "Select message with the specific attribute: [n]ew, [r]ead, [u]nread.", 0 },
  {"align",  't', NULL,   0, "Try to align", 0},
  {0, 0, 0, 0}
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      dbug++;
      break;

    case 'f':
      show_field = arg;
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
      
    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  NULL,
  doc,
  NULL,
  NULL, NULL
};

static const char *frm_argp_capa[] = {
  "common",
  "licence",
  "mailbox",
  NULL
};

/* Retrieve the Personal Name from the header To: or From:  */
static int
get_personal (header_t hdr, const char *field, char *personal, size_t buflen)
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
  
  mu_argp_parse (&argp, &argc, &argv, 0, frm_argp_capa, &c, NULL);

  /* have an argument */
  argc -= c;
  argv += c;

  if (argc)
    mailbox_name = argv[0];

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

    status = mailbox_create_default (&mbox, mailbox_name);

    if (status != 0)
      {
	fprintf (stderr, "could not create mailbox <%s>: %s\n",
	    mailbox_name ? mailbox_name : "default",
	    mu_errstring(status));
	goto cleanup;
      }

    if (dbug)
      {
	mu_debug_t debug;
	mailbox_get_debug (mbox, &debug);
	mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
      }

    status = mailbox_open (mbox, MU_STREAM_READ);

    if (status != 0)
      {
	fprintf (stderr, "could not open mailbox <%s>: %s\n",
	    mailbox_name ? mailbox_name : "default",
	    mu_errstring(status));
	goto cleanup;
      }

    if (! be_quiet)
      {
	observer_create (&observer, mbox);
	observer_set_action (observer, action, mbox);
	mailbox_get_observable (mbox, &observable);
	observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);
      }

    status = mailbox_scan (mbox, 1, &total);

    if (status != 0)
      {
	fprintf (stderr, "could not scan mailbox <%s>: %s\n",
	    mailbox_name ? mailbox_name : "default",
	    mu_errstring(status));
	goto cleanup;
      }

    if (! be_quiet)
      {
	observable_detach (observable, observer);
	observer_destroy (&observer, mbox);
      }
cleanup:
    mailbox_close(mbox);
    mailbox_destroy(&mbox);

    if(status != 0)
      return 3;
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

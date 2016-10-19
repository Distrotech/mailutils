/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007-2012, 2014-2016 Free Software
   Foundation, Inc.

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

#include <frm.h>

static int show_summary;   /* Summarize the number of messages by message
			      status in each mailbox. -S option */
static int be_quiet;       /* Quiet mode. -q option. */
static int show_query;     /* Additional flag toggled by -q to display 
			      a one-line summary for each mailbox */
static int align = 0;      /* Tidy mode. -t option. */

#define IS_READ 0x001
#define IS_OLD  0x010
#define IS_NEW  0x100
static int select_attribute;

static struct attr_tab {
  char *name;      /* Attribute name */
  int code;        /* Corresponding IS_.* flag */
  size_t len;      /* Minimum abbreviation length */
} attr_tab[] = {
  { "new", IS_NEW, 0 },
  { "old", IS_OLD, 0 },
  { "unread", IS_OLD, 0 },
  { "read", IS_READ, 0 },
  { NULL }
};


/* Attribute table handling */

/* Prepares the table for use. Computes minimum abbreviation lengths. */
static void
prepare_attrs (void)
{
  struct attr_tab *p, *q;

  for (p = attr_tab; p->name; p++)
    {
      const char *name = p->name;
      size_t len = strlen (name);
      size_t n = 1;

      for (q = attr_tab; q->name; q++)
	{
	  if (p != q)
	    {
	      const char *str = q->name;
	      size_t slen = strlen (str);

	      if (memcmp (name, str, n) == 0)
		{
		  for (n++;
		       memcmp (name, str, n) == 0
			 && n < len
			 && n < slen; n++)
		    ;
		  
		  q->len = n < slen ? n : slen;
		}
	    }
	}
      p->len = n < len ? n : len;
    }
}

/* Translates the textual status representation to the corresponding
   IS_.* flag */
static int
decode_attr (char const *arg)
{
  struct attr_tab *p;
  int len = strlen (arg);
  int pretendents = 0;
  
  for (p = attr_tab; p->name; p++)
    {
      const char *str = p->name;
      if (str[0] == arg[0])
	{
	  if (len < p->len)
	    pretendents++;
	  else if (len > strlen (str))
	    continue;
	  if (memcmp (str, arg, p->len) == 0)
	    return p->code;
	}
    }
  if (pretendents)
    mu_error (_("%s: ambiguous abbreviation"), arg);
  else
    mu_error (_("%s: unknown attribute"), arg);
  return 0;
}


static void
cli_show_field (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  show_field = mu_strdup (arg);
  align = 0;
}

static void
cli_Quiet (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  be_quiet += 2;
}

static void
cli_query (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  be_quiet++;
  show_query = 1;
}

static void
cli_status (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  select_attribute = decode_attr (arg);
}

static struct mu_option frm_options[] = {
  { "debug",  'd', NULL,   MU_OPTION_DEFAULT,
    N_("enable debugging output"),
    mu_c_incr, &frm_debug },
  { "field",  'f', N_("NAME"), MU_OPTION_DEFAULT,
    N_("header field to display"),
    mu_c_string, &show_field, cli_show_field },
  { "to",     'l', NULL,   MU_OPTION_DEFAULT,
    N_("include the To: information"),
    mu_c_bool, &show_to },
  { "number", 'n', NULL,   MU_OPTION_DEFAULT,
    N_("display message numbers"),
    mu_c_bool, &show_number },
  { "Quiet",  'Q', NULL,   MU_OPTION_DEFAULT,
    N_("do not display headers"),
    mu_c_int, &be_quiet, cli_Quiet },
  { "query",  'q', NULL,   MU_OPTION_DEFAULT,
    N_("print a message if the mailbox contains some unread mail"),
    mu_c_int, &be_quiet, cli_query },
  { "summary",'S', NULL,   MU_OPTION_DEFAULT,
    N_("print a summary of messages"), 
    mu_c_bool, &show_summary },
  { "status", 's', N_("STATUS"), 0,
    /* TRANSLATORS: Please do *not* translate the words "new", "unread",
       "old" and "read". They are keywords. */
    N_("select messages with the specific attribute:"
       " new, unread, old (same as unread) or read (or any unambiguous"
       " abbreviation of these)"),
    mu_c_string, NULL, cli_status},
  { "align",  't', NULL,   MU_OPTION_DEFAULT,
    N_("tidy mode: align subject lines"),
    mu_c_bool, &align },
  MU_OPTION_END
}, *options[] = { frm_options, NULL };

static struct mu_cli_setup cli = {
  options,
  NULL,
  N_("GNU frm -- display From: lines."),
  N_("[URL ...]"),
};

static char *frm_argp_capa[] = {
  "debug",
  "mailbox",
  "locking",
  NULL
};

static struct
{
  size_t new;
  size_t read;
  size_t unread;
} counter;


static int selected;

static int
frm_select (size_t index, mu_message_t msg)
{
  mu_header_t hdr = NULL;
  mu_attribute_t attr = NULL;
  
  mu_message_get_attribute (msg, &attr);
  mu_message_get_header (msg, &hdr);

  if (mu_attribute_is_read (attr))
    counter.read++;
  else if (mu_attribute_is_seen (attr))
    counter.unread++;
  else if (mu_attribute_is_recent (attr))
    counter.new++;

  if (((select_attribute & IS_READ) && (!mu_attribute_is_read (attr)))
      || ((select_attribute & IS_NEW) && (!mu_attribute_is_recent (attr)))
      || ((select_attribute & IS_OLD) && (!mu_attribute_is_seen (attr))))
    return 0;
  
  if (select_attribute)
    selected++;

  return !be_quiet;
}


/* This is a clone of the elm program call "frm".  It is a good example on
   how to use the observable(callback) of libmailbox.  "frm" has to
   be very interactive, it is not possible to call mu_mailbox_messages_count()
   and wait for the scanning to finish before displaying.  As soon as the scan
   finds a new message we want to know about it. This is done by registering
   an observable type MU_MAILBOX_MSG_ADD. The rest is formatting code.  */

int
frm (char *mailbox_name)
{
  size_t total;
  int status;

  selected = 0;
  counter.new = counter.read = counter.unread = 0;
  frm_scan (mailbox_name, frm_select, &total);
  
  if (show_summary)
    {
      if (total == 0)
	mu_printf (_("Folder contains no messages."));
      else
	{
	  char *delim = "";
      	  
	  mu_printf (_("Folder contains "));
	
	  if (counter.new)
	    {
	      mu_printf (ngettext ("%lu new message",
 				"%lu new messages",
				counter.new),
		      (u_long) counter.new);
	      delim = ", ";
	    }
	  
	  if (counter.unread)
	    {
	      mu_printf ("%s", delim);
	      
	      mu_printf (ngettext ("%lu unread message",
				"%lu unread messages",
				counter.unread),
		      (u_long) counter.unread);
	      delim = ", ";
	    }
	  
	  if (counter.read)
	    {
	      mu_printf ("%s", delim);
	      
	      mu_printf (ngettext ("%lu read message",
				"%lu read messages",
				counter.read),
		      (u_long) counter.read);
	    }
	  /* TRANSLATORS: This dot finishes the sentence

              "Folder contains XXX messages."

	      Leave it as it is unless your language requires to reorder
	      the parts of speach in the message
	  */
	  mu_printf (_("."));
	}
      mu_printf ("\n");
    }
  else if (show_query)
    {
      if (total > 0)
	mu_printf (_("There are messages in that folder.\n"));
      else
	mu_printf (_("No messages in that folder!\n"));
    }
  
  /* EXIT STATUS
     Frm returns a zero status ("true") if messages matching `status' are
     present.  Frm returns 1 if no messages matching `status' are present,
     but there are some messages, returns 2 if there are no messages at
     all, or returns 3 if an error occurred. */
  
  if (selected)
    status = 0;
  else if (total > 0)
    status = 1;
  else
    status = 2;

  return status;
}

int
main (int argc, char **argv)
{
  int status = 0;
  size_t s;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  prepare_attrs ();
  
  /* register the formats.  */
  mu_register_all_mbox_formats ();

  mu_auth_register_module (&mu_auth_tls_module);
  mu_cli (argc, argv, &cli, frm_argp_capa, NULL, &argc, &argv);

  if (align && (s = util_getcols ()))
    init_output (s);
  else
    init_output (0);
  
  /* have an argument */
  if (argc == 0)
    status = frm (NULL);
  else if (argc == 1)
    status = frm (argv[0]);
  else
    {
      int i;
      
      for (i = 0; i < argc; i++)
	{
	  mu_printf ("%s:\n", argv[i]);
	  status = frm (argv[i]);
	}
    }
  
  return status;
}

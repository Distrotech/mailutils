/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* MH scan command */

#include <mh.h>
#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <mailutils/observer.h>

const char *argp_program_version = "scan (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH scan\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder] [msgs]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify folder to scan")},
  {"clear",   ARG_CLEAR, N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("Clear screen after displaying the list")},
  {"form",    ARG_FORM, N_("FILE"),   0,
   N_("Read format from given file")},
  {"format",  ARG_FORMAT, N_("FORMAT"), 0,
   N_("Use this format string")},
  {"header",  ARG_HEADER, N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("Display header")},
  {"width",   ARG_WIDTH, N_("NUMBER"), 0,
   N_("Set output width")},
  {"reverse", ARG_REVERSE, N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("List messages in reverse order")},
  {"file",    ARG_FILE, N_("FILE"),   0,
   N_("[Not yet implemented]")},
  
  {"license", ARG_LICENSE, 0,        0,
   N_("Display software license"), -1},

  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"clear",   1,  MH_OPT_BOOL},
  {"form",    4,  MH_OPT_ARG, "formatfile"},
  {"format",  5,  MH_OPT_ARG, "string"},
  {"header",  3,  MH_OPT_BOOL},
  {"width",   1,  MH_OPT_ARG, "number"},
  {"reverse", 1,  MH_OPT_BOOL},
  {"file",    4,  MH_OPT_ARG, "file"},
  { 0 }
};

static int clear;
static char *format_str = mh_list_format;

static int width = 80;
static int reverse;
static int header;

static mh_format_t format;

static mh_msgset_t msgset;

void list_message __P((mailbox_t mbox, message_t msg, size_t num, void *data));
void print_header __P((mailbox_t mbox));
void clear_screen __P((void));

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      current_folder = arg;
      break;
      
    case ARG_CLEAR:
      clear = is_true(arg);
      break;
      
    case ARG_FORM:
      mh_read_formfile (arg, &format_str);
      break;
      
    case ARG_FORMAT:
      format_str = arg;
      break;
      
    case ARG_HEADER:
      header = is_true(arg);
      break;
      
    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("Invalid width"));
	  exit (1);
	}
      break;
      
    case ARG_REVERSE:
      reverse = is_true(arg);
      break;
      
    case ARG_FILE:
      argp_error (state, _("option is not yet implemented"));
      break;
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;
      
    default:
      return 1;
    }
  return 0;
}

/* Observable Action this is being call at every message discover.  */
static int
action (observer_t o, size_t type)
{
  static int counter;
  mailbox_t mbox;
  message_t msg = NULL;
  size_t num;

  if (type == MU_EVT_MESSAGE_ADD)
    {
      mbox = observer_get_owner (o);
      counter++;
      mailbox_get_message (mbox, counter, &msg);
      mh_message_number (msg, &num);
      list_message (mbox, msg, num, NULL);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int index;
  mailbox_t mbox;
  int status;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  if (mh_format_parse (format_str, &format))
    {
      mh_error (_("Bad format string"));
      exit (1);
    }

  mbox = mh_open_folder (current_folder, 0);

  argc -= index;
  argv += index;
  if ((argc == 0 || strcmp (argv[0], "all") == 0) && !reverse)
    {
      /* Fast approach */
      observer_t observer;
      observable_t observable;
      size_t total;
        
      print_header (mbox);
      
      observer_create (&observer, mbox);
      observer_set_action (observer, action, mbox);
      mailbox_get_observable (mbox, &observable);
      observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);

      status = mailbox_scan (mbox, 1, &total);
    }
  else
    {
      mh_msgset_parse (mbox, &msgset, argc, argv, "all");
  
      if (reverse)
	mh_msgset_reverse (&msgset);

      print_header (mbox);
      status = mh_iterate (mbox, &msgset, list_message, NULL);
    }

  clear_screen ();
  mh_global_save_state ();
  
  return status;
}

void
print_header (mailbox_t mbox)
{
  if (header)
    {
      url_t url = NULL;
      char datestr[64];
      time_t t;
	  
      mailbox_get_url (mbox, &url);
      time (&t);
      strftime (datestr, sizeof datestr, "%c", localtime (&t));
      printf (_("Folder %s  %s\n"), url_to_string (url), datestr);
    }
}

#ifdef HAVE_TERMCAP_H
/* A subroutine for tputs */
int
putstdout(char c)
{
  return putc(c, stdout);
}
#endif

void
clear_screen ()
{
  if (clear)
    {
#ifdef HAVE_TERMCAP_H
      if (isatty (1))
	{
	  char termcap_buf[1024];
	  char *buffer = termcap_buf;
	  char *termname;
      
	  if ((termname = getenv("TERM")) == NULL)
	    /* No terminal; Try ansi */
	    termname = "ansi";
      
	  if (tgetent(termcap_buf, termname) == 1)
	    {
	      char *clr = tgetstr ("cl", &buffer);
	      if (clr)
		{
		  tputs(clr, 1, (int (*)())putstdout);
		  return;
		}
	    }
	}
#endif
      /* Fall back to formfeed */
      fprintf (stdout, "\f");
    }
}

void
list_message (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  char *buffer;
  mh_format (&format, msg, num, width, &buffer);
  printf ("%s\n", buffer);
  free (buffer);
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH scan command */

#include <mh.h>
#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

const char *argp_program_version = "scan (" PACKAGE_STRING ")";
static char doc[] = "GNU MH scan";
static char args_doc[] = N_("[+folder] [msgs]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  'f', N_("FOLDER"), 0, N_("Specify folder to scan")},
  {"clear",   'c', N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("Clear screen after displaying the list")},
  {"form",    'F', N_("FILE"),   0, N_("Read format from given file")},
  {"format",  't', N_("FORMAT"), 0, N_("Use this format string")},
  {"header",  'H', N_("BOOL"),   OPTION_ARG_OPTIONAL, N_("Display header")},
  {"width",   'w', N_("NUMBER"), 0, N_("Set output width")},
  {"reverse", 'r', N_("BOOL"),   OPTION_ARG_OPTIONAL, N_("List messages in reverse order")},
  {"file",    'i', N_("FILE"),   0, N_("[Not yet implemented]")},
  
  {"license", 'l', 0,        0, N_("Display software license"), -1},

  { N_("\nUse -help switch to obtain the list of traditional MH options. "),
   0, 0, OPTION_DOC, "" },
  
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"clear",   1,  'c', MH_OPT_BOOL},
  {"form",    4,  'F', MH_OPT_ARG, "formatfile"},
  {"format",  5,  't', MH_OPT_ARG, "string"},
  {"header",  3,  'H', MH_OPT_BOOL},
  {"width",   1,  'w', MH_OPT_ARG, "number"},
  {"reverse", 1,  'r', MH_OPT_BOOL},
  {"file",    4,  'i', MH_OPT_ARG, "file"},
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
int scan __P((mailbox_t mbox));

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case '+':
    case 'f': 
      current_folder = arg;
      break;
      
    case 'c':
      clear = is_true(arg);
      break;
      
    case 'F':
      mh_read_formfile (arg, &format_str);
      break;
      
    case 't':
      format_str = arg;
      break;
      
    case 'H':
      header = is_true(arg);
      break;
      
    case 'w':
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  mh_error (_("Invalid width"));
	  exit (1);
	}
      break;
      
    case 'r':
      reverse = is_true(arg);
      break;
      
    case 'i':
      mh_error (_("'i' is not yet implemented."));
      break;
      
    case 'l':
      mh_license (argp_program_version);
      break;
      
    default:
      return 1;
    }
  return 0;
}


int
main (int argc, char **argv)
{
  int index;
  mailbox_t mbox;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  if (mh_format_parse (format_str, &format))
    {
      mh_error (_("Bad format string"));
      exit (1);
    }

  mbox = mh_open_folder (current_folder, 0);
  mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "all");
  
  if (reverse)
    mh_msgset_reverse (&msgset);

  return scan (mbox);
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

struct list_data {
  char *buffer;
  size_t width;
};

int
scan (mailbox_t mbox)
{
  struct list_data list_data;
  
  list_data.buffer = xmalloc (width);
  list_data.width  = width;

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

  mh_iterate (mbox, &msgset, list_message, &list_data);
  
  clear_screen ();
  mh_global_save_state ();
  return 0;
}

void
list_message (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  struct list_data *ld = data;
  
  ld->buffer[0] = 0;
  mh_format (&format, msg, num, ld->buffer, ld->width);
  printf ("%s\n", ld->buffer);
}

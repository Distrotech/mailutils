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
static char args_doc[] = "";

/* GNU options */
static struct argp_option options[] = {
  {"folder",  'f', "FOLDER", 0, "Specify folder to scan"},
  {"clear",   'c', "BOOL",   OPTION_ARG_OPTIONAL, "Clear screen after displaying the list"},
  {"form",    'F', "FILE",   0, "Read format from given file"},
  {"format",  't', "FORMAT", 0, "Use this format string"},
  {"header",  'H', "BOOL",   OPTION_ARG_OPTIONAL, "Display header"},
  {"width",   'w', "NUMBER", 0, "Set output width"},
  {"reverse", 'r', "BOOL",   OPTION_ARG_OPTIONAL, "List messages in reverse order"},
  {"file",    'i', "FILE",   0, "[Not yet implemented]"},
  
  {"license", 'l', 0,        0, "Display software license", -1},

  { "\nUse -help switch to obtain the list of traditional MH options. ", 0, 0, OPTION_DOC, "" },
  
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

void list_message (mailbox_t mbox, size_t msgno, char *bp, size_t width);
int scan (mailbox_t mbox);

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
      clear = arg[0] == 'y';
      break;
      
    case 'F':
      mh_read_formfile (arg, &format_str);
      break;
      
    case 't':
      format_str = arg;
      break;
      
    case 'H':
      header = arg[0] == 'y';
      break;
      
    case 'w':
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  mh_error ("Invalid width");
	  exit (1);
	}
      break;
      
    case 'r':
      reverse = arg[0] == 'y';
      break;
      
    case 'i':
      mh_error ("'i' is not yet implemented.");
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
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);

  if (mh_format_parse (format_str, &format))
    {
      mh_error ("Bad format string");
      exit (1);
    }

  return scan (mh_open_folder (current_folder, 0));
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

int
scan (mailbox_t mbox)
{
  size_t i, total;
  char *buffer;
  
  buffer = xmalloc (width);

  if (header)
    {
      url_t url = NULL;
      char datestr[64];
      time_t t;
      
      mailbox_get_url (mbox, &url);
      time (&t);
      strftime (datestr, sizeof datestr, "%c", localtime (&t));
      printf ("Folder %s  %s\n", url_to_string (url), datestr);
    }
  
  mailbox_messages_count (mbox, &total);

  if (reverse)
    for (i = total; i >= 1; i--)
      list_message (mbox, i, buffer, width);
  else
    for (i = 1; i <= total; i++)
      list_message (mbox, i, buffer, width);
  clear_screen ();
  return 0;
}

void
list_message (mailbox_t mbox, size_t msgno, char *buffer, size_t width)
{
  message_t msg;

  buffer[0] = 0;
  mailbox_get_message (mbox, msgno, &msg);
  mh_format (&format, msg, msgno, buffer, width);
  printf ("%s\n", buffer);
}

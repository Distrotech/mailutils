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

const char *argp_program_version = "scan (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
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

static char *scan_folder = "inbox";
static int clear;
static char *formfile;
static char *format_str = 
"%4(msg)%<(cur)+%| %>%<{replied}-%?{encrypted}E%| %>"
"%02(mon{date})/%02(mday{date})"
"%<{date} %|*%>"
"%<(mymbox{from})%<{to}To:%14(friendly{to})%>%>"
"%<(zero)%17(friendly{from})%>"
"  %{subject}%<{body}<<%{body}>>%>";

static int width = 80;
static int reverse;

static mh_format_t format;

void list_message (mailbox_t mbox, size_t msgno, char *bp, size_t width);
int scan (mailbox_t mbox);

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case 'f': 
      scan_folder = arg;
      break;
      
    case 'c':
      clear = arg[0] == 'y';
      break;
      
    case 'F':
      formfile = arg;
      break;
      
    case 't':
      format_str = arg;
      break;
      
    case 'H':
      clear = arg[0] == 'y';
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

void
set_folder ()
{
  scan_folder = mu_tilde_expansion (scan_folder, "/", NULL);
  if (strchr (scan_folder, '/') == NULL)
    {
      char *p = mu_get_homedir ();
      asprintf (&scan_folder, "mh:%s/Mail/%s", p, scan_folder);
      if (!scan_folder)
	{
	  mh_error ("low memory");
	  exit (1);
	}
    }
  else if (strchr (scan_folder, ':') == NULL)
    {
      char *p;
      p = xmalloc (strlen (scan_folder) + 4);
      strcat (strcpy (p, "mh:"), scan_folder);
      scan_folder = p;
    }
}

int
main (int argc, char **argv)
{
  mailbox_t mbox = NULL;

  mh_init ();
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL);

  set_folder ();
  if (mh_format_parse (format_str, &format))
    {
      mh_error ("Bad format string");
      exit (1);
    }

  if (mailbox_create_default (&mbox, scan_folder))
    {
      mh_error ("Can't create mailbox %s: %s", scan_folder, strerror (errno));
      exit (1);
    }

  if (mailbox_open (mbox, MU_STREAM_READ))
    {
      mh_error ("Can't open mailbox %s: %s", scan_folder, strerror (errno));
      exit (1);
    }

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
#ifdef HAVE_TERMCAP_H
  if (clear)
    {
      char termcap_buf[1024];
      char *buffer;
      char *termname;
      
      if ((termname = getenv("TERM")) == NULL)
	  /* No terminal; Try ansi */
	termname = "ansi";
      
      if (tgetent(termcap_buf, termname) != 1)
	fprintf (stdout, "\f");
      else
	{
	  char *clr = tgetstr ("cl", &buffer);
	  if (clr)
	    tputs(clr, 1, (int (*)())putstdout);
	  else
	    fprintf (stdout, "\f");
	}
    }
#endif
}

int
scan (mailbox_t mbox)
{
  size_t i, total;
  char *buffer;

  buffer = xmalloc (width);
  
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

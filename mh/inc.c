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

/* MH inc command */

#include <mh.h>

const char *argp_program_version = "inc (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU MH inc";
static char args_doc[] = "";

/* GNU options */
static struct argp_option options[] = {
  {"file",    'i', "FILE",   0, "Inorporate mail from named file"},
  {"folder",  'f', "FOLDER", 0, "Specify folder to incorporate mail to"},
  {"audit",   'a', "FILE", 0, "Enable audit"},
  {"noaudit", 'n', 0, 0, "Disable audit"},
  {"changecur", 'c', "BOOL", OPTION_ARG_OPTIONAL, "Mark first incorporated message as current (default)"},
  {"form",    'F', "FILE",   0, "Read format from given file"},
  {"format",  't', "FORMAT", 0, "Use this format string"},
  {"truncate", 'T', "BOOL", OPTION_ARG_OPTIONAL, "Truncate source mailbox after incorporating (default)"},

  {"width",   'w', "NUMBER", 0, "Set output width"},

  {"quiet",   'q', 0,        0, "Be quiet"},
  {"license", 'l', 0,        0, "Display software license", -1},

  { "\nUse -help switch to obtain the list of traditional MH options. ", 0, 0, OPTION_DOC, "" },
  
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"audit",   5,  'a', MH_OPT_ARG, "audit-file"},
  {"noaudit", 3,  'n', 0, },
  {"changecur", 1, 'c',MH_OPT_BOOL},
  {"file",    2, 'i', MH_OPT_ARG, "input-file"},
  {"form",    4, 'F', MH_OPT_ARG, "format-file"},
  {"format",  5, 't', MH_OPT_ARG, "string"},
  {"truncate", 2, 'T', MH_OPT_BOOL, },
  {"width",   1,  'w', MH_OPT_ARG, "number"},
  {"quiet",   1,  'q', 0, },
  { 0 }
};

static char *format_str = mh_list_format;
static int width = 80;
static char *input_file;
static char *audit_file; 
static FILE *audit_fp;
static int changecur = -1;
static int truncate = -1;
static int quiet = 0;

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case 'a':
      audit_file = arg;
      break;

    case 'n':
      audit_file = NULL;
      break;
      
    case 'c':
      changecur = arg[0] == 'y';
      break;

    case '+':
    case 'f': 
      current_folder = arg;
      break;
      
    case 'F':
      mh_read_formfile (arg, &format_str);
      break;
      
    case 'i':
      input_file = arg;
      break;

    case 'T':
      truncate = arg[0] == 'y';
      break;
      
    case 'w':
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  mh_error ("Invalid width");
	  exit (1);
	}
      break;

    case 'q':
      quiet = 1;
      break;
      
    default:
      return 1;
    }
  return 0;
}

void
list_message (mh_format_t *format, mailbox_t mbox, size_t msgno,
	      char *buffer, size_t width)
{
  message_t msg;

  buffer[0] = 0;
  mailbox_get_message (mbox, msgno, &msg);
  mh_format (format, msg, msgno, buffer, width);
  printf ("%s\n", buffer);
  if (audit_fp)
    fprintf (audit_fp, "%s\n", buffer);
}

int
main (int argc, char **argv)
{
  mailbox_t input = NULL;
  mailbox_t output = NULL;
  size_t total, n;
  size_t lastmsg;
  int f_truncate = 0;
  int f_changecur = 0;
  char *buffer;
  mh_format_t format;
  
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL);

  if (!quiet && mh_format_parse (format_str, &format))
    {
      mh_error ("Bad format string");
      exit (1);
    }

  /* Select and open input mailbox */
  if (input_file == NULL)
    {
      if (mailbox_create_default (&input, NULL) != 0)
	{
	  mh_error ("Can not create default mailbox", strerror (errno));
	  exit (1);
	}
      f_truncate = 1;
      f_changecur = 1;
    }
  else if (mailbox_create_default (&input, input_file) != 0)
    {
      mh_error ("Can not create mailbox %s: %s", input_file, strerror (errno));
      exit (1);
    }

  if (mailbox_open (input, MU_STREAM_RDWR) != 0)
    {
      url_t url;
      mailbox_get_url (input, &url);
      mh_error ("Can not open mailbox %s: %s", url_to_string (url),
		strerror (errno));
      exit (1);
    }

  if (mailbox_messages_count (input, &total) != 0)
    {
      mh_error ("Can not read input mailbox");
      exit (1);
    }
  
  /* Select and open output mailbox */
  if (mh_check_folder (current_folder))
    exit (0);
  
  if (mailbox_create_default (&output, current_folder))
    {
      mh_error ("Can't create output mailbox %s: %s",
		current_folder, strerror (errno));
      exit (1);
    }

  if (mailbox_open (output, MU_STREAM_RDWR|MU_STREAM_CREAT))
    {
      mh_error ("Can't open mailbox %s: %s", current_folder,
		strerror (errno));
      exit (1);
    }

  if (mailbox_messages_count (output, &lastmsg) != 0)
    {
      mh_error ("Can not read output mailbox");
      exit (1);
    }

  buffer = xmalloc (width);
  
  /* Fixup options */
  if (truncate == -1)
    truncate = f_truncate;
  if (changecur == -1)
    changecur = f_changecur;

  /* Open audit file, if specified */
  if (audit_file)
    audit_fp = mh_audit_open (audit_file, input);
  
  for (n = 1; n <= total; n++)
    {
      message_t imsg;
      
      if (mailbox_get_message (input, n, &imsg))
	{
	  mh_error ("%d: can't get message: %s", n, strerror (errno));
	  continue;
	}

      if (mailbox_append_message (output, imsg))
	{
	  mh_error ("%d: error appending message: %s",
		    n, strerror (errno));
	  continue;
	}

      if (n == 1 && changecur)
	{
	  message_t msg;
      
	  mailbox_get_message (output, lastmsg+1, &msg);
	  mh_message_number (msg, &current_message);
	}
	  
      if (!quiet)
	list_message (&format, output, lastmsg + n, buffer, width);
      
      if (truncate)
	{
	  attribute_t attr;
	  message_get_attribute (imsg, &attr);
	  attribute_set_deleted (attr);
	}
    }

  if (changecur)
    mh_save_context ();
  
  mailbox_close (output);
  mailbox_destroy (&output);

  if (truncate)
    mailbox_expunge (input);
  mailbox_close (input);
  mailbox_destroy (&input);

  if (audit_fp)
    mh_audit_close (audit_fp);
  
  return 0;
}


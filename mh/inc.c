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

/* MH inc command */

#include <mh.h>

const char *argp_program_version = "inc (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH inc\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder]");

/* GNU options */
static struct argp_option options[] = {
  {"file",    'i', N_("FILE"),   0, N_("Incorporate mail from named file")},
  {"folder",  'f', N_("FOLDER"), 0, N_("Specify folder to incorporate mail to")},
  {"audit",   'a', N_("FILE"), 0, N_("Enable audit")},
  {"noaudit", 'n', 0, 0, N_("Disable audit")},
  {"changecur", 'c', N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Mark first incorporated message as current (default)")},
  {"form",    'F', N_("FILE"),   0, N_("Read format from given file")},
  {"format",  't', N_("FORMAT"), 0, N_("Use this format string")},
  {"truncate", 'T', N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Truncate source mailbox after incorporating (default)")},
  {"width",   'w', N_("NUMBER"), 0, N_("Set output width")},
  {"quiet",   'q', 0,        0, N_("Be quiet")},
  {"license", 'l', 0,        0, N_("Display software license"), -1},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"audit",     5, MH_OPT_ARG, "audit-file" },
  {"noaudit",   3, 0, },
  {"changecur", 1, MH_OPT_BOOL},
  {"file",      2, MH_OPT_ARG, "input-file"},
  {"form",      4, MH_OPT_ARG, "format-file"},
  {"format",    5, MH_OPT_ARG, "string"},
  {"truncate",  2, MH_OPT_BOOL, },
  {"width",     1, MH_OPT_ARG, "number"},
  {"quiet",     1, 0, },
  { 0 }
};

static char *format_str = mh_list_format;
static int width = 80;
static char *input_file;
static char *audit_file; 
static FILE *audit_fp;
static int changecur = -1;
static int truncate_source = -1;
static int quiet = 0;
static char *append_folder;

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case ARGP_KEY_FINI:
      if (!append_folder)
	append_folder = mh_global_profile_get ("Inbox", "inbox");
      break;

    case 'a':
      audit_file = arg;
      break;

    case 'n':
      audit_file = NULL;
      break;
      
    case 'c':
      changecur = is_true(arg);
      break;

    case '+':
    case 'f': 
      append_folder = arg;
      break;
      
    case 'F':
      mh_read_formfile (arg, &format_str);
      break;
      
    case 'i':
      input_file = arg;
      break;

    case 'T':
      truncate_source = is_true(arg);
      break;

    case 'w':
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  mh_error (_("Invalid width"));
	  exit (1);
	}
      break;

    case 'q':
      quiet = 1;
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
list_message (mh_format_t *format, mailbox_t mbox, size_t msgno, size_t width)
{
  message_t msg;
  char *buf = NULL;

  mailbox_get_message (mbox, msgno, &msg);
  mh_format (format, msg, msgno, width, &buf);
  printf ("%s\n", buf);
  if (audit_fp)
    fprintf (audit_fp, "%s\n", buf);
  free (buf);
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
  mh_format_t format;
  int rc;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);

  if (!quiet && mh_format_parse (format_str, &format))
    {
      mh_error (_("Bad format string"));
      exit (1);
    }

  /* Select and open input mailbox */
  if (input_file == NULL)
    {
      if ((rc = mailbox_create_default (&input, NULL)) != 0)
	{
	  mh_error (_("Can not create default mailbox"),
		    mu_strerror (rc));
	  exit (1);
	}
      f_truncate = 1;
      f_changecur = 1;
    }
  else if ((rc = mailbox_create_default (&input, input_file)) != 0)
    {
      mh_error (_("Can not create mailbox %s: %s"),
		input_file, mu_strerror (rc));
      exit (1);
    }

  if ((rc = mailbox_open (input, MU_STREAM_RDWR)) != 0)
    {
      url_t url;
      mailbox_get_url (input, &url);
      mh_error (_("Can not open mailbox %s: %s"),
		url_to_string (url),
		mu_strerror (errno));
      exit (1);
    }

  if ((rc = mailbox_messages_count (input, &total)) != 0)
    {
      mh_error (_("Can not read input mailbox: %s"), mu_strerror (errno));
      exit (1);
    }

  output = mh_open_folder (append_folder, 1);
  if ((rc = mailbox_messages_count (output, &lastmsg)) != 0)
    {
      mh_error (_("Can not read output mailbox: %s"),
		mu_strerror (errno));
      exit (1);
    }
  
  /* Fixup options */
  if (truncate_source == -1)
    truncate_source = f_truncate;
  if (changecur == -1)
    changecur = f_changecur;

  /* Open audit file, if specified */
  if (audit_file)
    audit_fp = mh_audit_open (audit_file, input);
  
  for (n = 1; n <= total; n++)
    {
      message_t imsg;
      
      if ((rc = mailbox_get_message (input, n, &imsg)) != 0)
	{
	  mh_error (_("%d: can't get message: %s"),
		    n, mu_strerror (errno));
	  continue;
	}

      if ((rc = mailbox_append_message (output, imsg)) != 0)
	{
	  mh_error (_("%d: error appending message: %s"),
		    n, mu_strerror (errno));
	  continue;
	}

      if (n == 1 && changecur)
	{
	  message_t msg = NULL;
      
	  mailbox_get_message (output, lastmsg+1, &msg);
	  mh_message_number (msg, &current_message);
	}
	  
      if (!quiet)
	list_message (&format, output, lastmsg + n, width);
      
      if (truncate_source)
	{
	  attribute_t attr;
	  message_get_attribute (imsg, &attr);
	  attribute_set_deleted (attr);
	}
    }

  if (changecur)
    mh_global_save_state ();
  
  mailbox_close (output);
  mailbox_destroy (&output);

  if (truncate_source)
    mailbox_expunge (input);
  mailbox_close (input);
  mailbox_destroy (&input);

  if (audit_fp)
    mh_audit_close (audit_fp);
  
  return 0;
}


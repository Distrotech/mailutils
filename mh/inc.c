/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2012, 2014-2015 Free Software
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

/* MH inc command */

#include <mh.h>
#include "muscript.h"

static char doc[] = N_("GNU MH inc")"\v"
N_("Debug flags are:\n\
  g - guile stack traces\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n\
  l - sieve action logs\n\n\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+FOLDER]");

/* GNU options */
static struct argp_option options[] = {
  {"file",    ARG_FILE, N_("FILE"),   0,
   N_("incorporate mail from named file")},
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("specify folder to incorporate mail to")},
  {"audit",   ARG_AUDIT, N_("FILE"), 0,
   N_("enable audit")},
  {"noaudit", ARG_NOAUDIT, 0, 0,
   N_("disable audit")},
  {"changecur", ARG_CHANGECUR, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("mark first incorporated message as current (default)")},
  {"nochangecur", ARG_NOCHANGECUR, NULL, OPTION_HIDDEN, ""},
  {"form",    ARG_FORM, N_("FILE"),   0,
   N_("read format from given file")},
  {"format",  ARG_FORMAT, N_("FORMAT"), 0,
   N_("use this format string")},
  {"truncate", ARG_TRUNCATE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("truncate source mailbox after incorporating (default)")},
  {"notruncate", ARG_NOTRUNCATE, NULL, OPTION_HIDDEN, ""},
  {"moveto",  ARG_MOVETO, N_("MAILBOX"), 0,
   N_("move incorporated messages to MAILBOX instead of deleting them") },
  {"width",   ARG_WIDTH, N_("NUMBER"), 0,
   N_("set output width")},
  {"quiet",   ARG_QUIET, 0,        0,
   N_("be quiet")},
  {"notify",  ARG_NOTIFY,N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("enable biff notification"), },
  {"language",ARG_LANG,  N_("LANG"), 0,
   N_("set language for the --script option") },
  {"script",  ARG_SCRIPT,N_("FILE"), 0,
   N_("filter incoming messages using script FILE") },
  {"debug",   ARG_DEBUG, N_("FLAGS"), 0,
   N_("enable debugging") },   
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "audit",     MH_OPT_ARG, "audit-file" },
  { "noaudit" },
  { "changecur", MH_OPT_BOOL },
  { "file",      MH_OPT_ARG, "input-file" },
  { "form",      MH_OPT_ARG, "format-file" },
  { "format",    MH_OPT_ARG, "string" },
  { "truncate",  MH_OPT_BOOL },
  { "moveto",    MH_OPT_ARG, "folder" },
  { "width",     MH_OPT_ARG, "number" },
  { "notify",    MH_OPT_BOOL },
  { "quiet" },
  { "language",  MH_OPT_ARG, "lang" },
  { "script",    MH_OPT_ARG, "file" },
  { "debug",     MH_OPT_ARG, "flags" },
  { NULL }
};

static char *format_str = mh_list_format;
static int width = 80;
static mu_list_t input_file_list;
static char *audit_file; 
static FILE *audit_fp;
static int changecur = -1;
static int truncate_source = -1;
static int quiet = 0;
static int notify = 0;
static const char *append_folder;
static const char *move_to_mailbox;
static const char *script_file;
static const char *script_lang;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  int rc;
  char *p;
  
  switch (key)
    {
    case ARGP_KEY_FINI:
      if (!append_folder)
	append_folder = mh_global_profile_get ("Inbox", "inbox");
      break;

    case ARG_AUDIT:
      audit_file = arg;
      break;

    case ARG_NOAUDIT:
      audit_file = NULL;
      break;
      
    case ARG_CHANGECUR:
      changecur = is_true (arg);
      break;

    case ARG_DEBUG:
      if (mu_script_debug_flags (arg, &p))
	argp_error (state, _("invalid debug flag near %s"), p);
      break;

    case ARG_NOCHANGECUR:
      changecur = 0;
      break;
      
    case ARG_FOLDER: 
      append_folder = arg;
      break;
      
    case ARG_FORM:
      mh_read_formfile (arg, &format_str);
      break;

    case ARG_FORMAT:
      format_str = arg;
      break;
      
    case ARG_FILE:
      if (!input_file_list)
	{
	  rc = mu_list_create (&input_file_list);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR,
			       "mu_list_create", "&input_file_list", rc);
	      exit (1);
	    }
	}
      rc = mu_list_append (input_file_list, arg);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", arg, rc);
	  exit (1);
	}
      break;

    case ARG_TRUNCATE:
      truncate_source = is_true (arg);
      break;

    case ARG_NOTRUNCATE:
      truncate_source = 0;
      break;

    case ARG_MOVETO:
      move_to_mailbox = arg;
      break;
      
    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("invalid width"));
	  exit (1);
	}
      break;

    case ARG_QUIET:
      quiet = 1;
      break;

    case ARG_NOTIFY:
      notify = is_true (arg);;
      break;

    case ARG_LANG:
      script_lang = arg;
      break;

    case ARG_SCRIPT:
      script_file = arg;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

void
list_message (mh_format_t *format, mu_mailbox_t mbox, size_t msgno,
	      size_t width)
{
  mu_message_t msg;
  char *buf = NULL;

  mu_mailbox_get_message (mbox, msgno, &msg);
  mh_format (format, msg, msgno, width, &buf);
  printf ("%s\n", buf);
  if (audit_fp)
    fprintf (audit_fp, "%s\n", buf);
  free (buf);
}

struct incdat
{
  mu_mailbox_t output;
  size_t lastmsg;
  mh_format_t format;
  mu_script_t handler;
  mu_script_descr_t descr;
};

static int
getparam (mu_url_t url, const char *param, const char **sval)
{
  int rc = mu_url_sget_param (url, param, sval);
  switch (rc)
    {
    case 0:
    case MU_ERR_NOENT:
      break;
	  
    default:
      mu_diag_funcall (MU_DIAG_ERROR, "mu_url_sget_param", param, rc);
      exit (1);
    }
  return rc;
}

static int
incmbx (void *item, void *data)
{
  char *input_file = item;
  struct incdat *dp = data;
  mu_mailbox_t input = NULL;
  size_t total, n;
  int rc;
  int f_truncate = truncate_source;
  const char *f_move_to_mailbox = move_to_mailbox, *sval;
  mu_url_t url;
  
  /* Select and open input mailbox */
  if (input_file == NULL)
    {
      if ((rc = mu_mailbox_create_default (&input, NULL)) != 0)
	{
	  mu_error (_("cannot create default mailbox: %s"),
		    mu_strerror (rc));
	  exit (1);
	}
    }
  else if ((rc = mu_mailbox_create_default (&input, input_file)) != 0)
    {
      mu_error (_("cannot create mailbox %s: %s"),
		input_file, mu_strerror (rc));
      exit (1);
    }
  
  if ((rc = mu_mailbox_open (input, MU_STREAM_RDWR)) != 0)
    {
      mu_url_t url;
      mu_mailbox_get_url (input, &url);
      mu_error (_("cannot open mailbox %s: %s"),
		mu_url_to_string (url),
		mu_strerror (rc));
      exit (1);
    }

  if ((rc = mu_mailbox_messages_count (input, &total)) != 0)
    {
      mu_error (_("cannot read input mailbox: %s"), mu_strerror (errno));
      exit (1);
    }

  if ((rc = mu_mailbox_get_url (input, &url)) != 0)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_url", input_file, rc);
      exit (1);
    }

  if (getparam (url, "truncate", &sval) == 0)
    {
      if (!sval)
	f_truncate = 1;
      else
	f_truncate = is_true (sval);
    }
  
  if (getparam (url, "nomoveto", NULL) == 0)
    f_move_to_mailbox = NULL;
  else
    getparam (url, "moveto", &f_move_to_mailbox);
  
  /* Open audit file, if specified */
  if (audit_file)
    audit_fp = mh_audit_open (audit_file, input);

  for (n = 1; n <= total; n++)
    {
      mu_message_t imsg;
      
      if ((rc = mu_mailbox_get_message (input, n, &imsg)) != 0)
	{
	  mu_error (_("%lu: cannot get message: %s"),
		    (unsigned long) n, mu_strerror (rc));
	  continue;
	}

      if (dp->handler)
	{
	  mu_attribute_t attr;

	  if (mu_script_process_msg (dp->handler, dp->descr, imsg))
	    {
	      mu_error (_("%lu: filter failed: %s"),
		    (unsigned long) n, mu_strerror (rc));
	      continue;
	    }

	  mu_message_get_attribute (imsg, &attr);
	  if (mu_attribute_is_deleted (attr))
	    continue;
	}

      
      if ((rc = mu_mailbox_append_message (dp->output, imsg)) != 0)
	{
	  mu_error (_("%lu: error appending message: %s"),
		    (unsigned long) n, mu_strerror (rc));
	  continue;
	}
      
      if (n == 1 && changecur)
	{
	  mu_message_t msg = NULL;
	  size_t cur;
	  
	  mu_mailbox_get_message (dp->output, dp->lastmsg + 1, &msg);
	  mh_message_number (msg, &cur);
	  mh_mailbox_set_cur (dp->output, cur);
	}

      ++dp->lastmsg;
      if (!quiet)
	list_message (&dp->format, dp->output, dp->lastmsg, width);
      
      if (f_truncate)
	{
	  mu_attribute_t attr;
	  mu_message_get_attribute (imsg, &attr);
	  mu_attribute_set_deleted (attr);
	}
    }

  if (total && f_truncate)
    {
      if (f_move_to_mailbox)
	{
	  mu_msgset_t msgset;
      
	  rc = mu_msgset_create (&msgset, input, MU_MSGSET_NUM);
	  if (rc)
	    mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_create", NULL, rc);
	  else
	    {
	      rc = mu_msgset_add_range (msgset, 1, total, MU_MSGSET_NUM);
	      if (rc)
		mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_add_range",
				 NULL, rc);
	      else
		{
		  rc = mu_mailbox_msgset_copy (input, msgset, f_move_to_mailbox,
					       MU_MAILBOX_COPY_CREAT);
		  if (rc)
		    {
		      mu_error (_("failed to move messages to %s: %s"),
				f_move_to_mailbox, mu_strerror (rc));
		      f_truncate = 0;
		    }
		}
	      mu_msgset_destroy (&msgset);
	    }
	}
      mu_mailbox_expunge (input);
    }
  
  if (audit_fp)
    {
      mh_audit_close (audit_fp);
      audit_fp = NULL;
    }

  mu_mailbox_close (input);
  mu_mailbox_destroy (&input);

  return 0;
}

int
main (int argc, char **argv)
{
  struct incdat incdat;
  int rc;
  int f_truncate = 0;
  int f_changecur = 0;
  size_t lastseen;
  const char *unseen_seq;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();
  
  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);
  mu_registrar_set_default_scheme ("mh");

  /* Inc sets missing cur to 1 */
  mh_mailbox_cur_default = 1;

  memset (&incdat, 0, sizeof (incdat));
  if (!quiet && mh_format_parse (format_str, &incdat.format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }

  incdat.output = mh_open_folder (append_folder,
				  MU_STREAM_READ|MU_STREAM_APPEND|MU_STREAM_CREAT);

  if (script_file)
    {
      if (script_lang)
	{
	  incdat.handler = mu_script_lang_handler (script_lang);
	  if (!incdat.handler)
	    {
	      mu_error (_("unknown or unsupported language: %s"),
			script_lang);
	      exit (1);
	    }
	}
      else
	{
	  incdat.handler = mu_script_suffix_handler (script_file);
	  if (!incdat.handler)
	    {
	      mu_error (_("unknown or unsupported language: %s"),
			script_file);
	      exit (1);
	    }
	}
      rc = mu_script_init (incdat.handler, script_file, &incdat.descr);
      if (rc)
	{
	  mu_error (_("script initialization failed: %s"),
		    mu_strerror (rc));
	  exit (1);
	}
    }
  
  if (notify)
    {
      rc = mu_mailbox_set_notify (incdat.output, NULL);
      if (rc)
	mu_error (_("failed to set up notification: %s"),
		  mu_strerror (rc));
    }
  
  if ((rc = mu_mailbox_messages_count (incdat.output, &incdat.lastmsg)) != 0)
    {
      mu_error (_("cannot read output mailbox: %s"),
		mu_strerror (errno));
      exit (1);
    }
  lastseen = incdat.lastmsg + 1;
  
  /* Fixup options */
  if (!input_file_list)
    f_truncate = f_changecur = 1;
  if (truncate_source == -1)
    truncate_source = f_truncate;
  if (changecur == -1)
    changecur = f_changecur;

  if (!input_file_list)
    incmbx (NULL, &incdat);
  else
    mu_list_foreach (input_file_list, incmbx, &incdat);

  if (script_file)
    mu_script_done (incdat.handler, incdat.descr);
  
  unseen_seq = mh_global_profile_get ("Unseen-Sequence", NULL);
  if (unseen_seq && lastseen < incdat.lastmsg)
    {
      mu_msgset_t unseen;

      mu_msgset_create (&unseen, NULL, MU_MSGSET_NUM);
      mu_msgset_add_range (unseen, lastseen, incdat.lastmsg, MU_MSGSET_NUM);
      mh_seq_add (incdat.output,
		  unseen_seq,
		  unseen, 0);
      mu_msgset_free (unseen);
    }
  
  mh_global_save_state ();
  
  mu_mailbox_close (incdat.output);
  mu_mailbox_destroy (&incdat.output);
  
  return 0;
}


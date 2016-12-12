/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2012, 2014-2016 Free Software
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

static char prog_doc[] = N_("Incorporate new mail");
static char extra_doc[] = N_("Debug flags are:\n\
  g - guile stack traces\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n\
  l - sieve action logs");

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
static char const *script_env[] = { "location=MUA", "phase=post", NULL };

static void
add_file (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  int rc;
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
  rc = mu_list_append (input_file_list, mu_strdup (arg));
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", arg, rc);
      exit (1);
    }
}

static void
set_debug (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  char *p;
  
  if (mu_script_debug_flags (arg, &p))
    {
      mu_parseopt_error (po, _("invalid debug flag near %s"), p);
      exit (po->po_exit_error);
    }
}

static struct mu_option options[] = {
  { "file",    0, N_("FILE"),   MU_OPTION_DEFAULT,
    N_("incorporate mail from named file"),
    mu_c_string, NULL, add_file },
  { "folder",  0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify folder to incorporate mail to"),
    mu_c_string, &append_folder },
  { "audit",   0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("enable audit"),
    mu_c_string, &audit_file },
  { "noaudit", 0, NULL, MU_OPTION_DEFAULT,
    N_("disable audit"),
    mu_c_string, &audit_file, mh_opt_clear_string },
  { "changecur", 0, NULL, MU_OPTION_DEFAULT,
    N_("mark first incorporated message as current (default)"),
    mu_c_bool, &changecur },
  { "form",      0, N_("FILE"),   MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, &format_str, mh_opt_read_formfile },
  { "format",    0, N_("FORMAT"), MU_OPTION_DEFAULT,
    N_("use this format string"),
    mu_c_string, &format_str },
  { "truncate",  0, NULL, MU_OPTION_DEFAULT,
    N_("truncate source mailbox after incorporating (default)"),
    mu_c_bool, &truncate_source },
  { "moveto",    0, N_("MAILBOX"), MU_OPTION_DEFAULT,
    N_("move incorporated messages to MAILBOX instead of deleting them"),
    mu_c_string, &move_to_mailbox },
  { "width",     0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output width"),
    mu_c_int, &width },
  { "quiet",     0, NULL,         MU_OPTION_DEFAULT,
    N_("be quiet"),
    mu_c_bool, &quiet },
  { "notify",    0, NULL,         MU_OPTION_DEFAULT,
    N_("enable biff notification"),
    mu_c_bool, &notify },
  { "language",  0, N_("LANG"),   MU_OPTION_DEFAULT,
    N_("set language for the --script option"),
    mu_c_string, &script_lang },
  { "script",    0, N_("FILE"),   MU_OPTION_DEFAULT,
    N_("filter incoming messages using script FILE"),
    mu_c_string, &script_file },
  { "debug",     0, N_("FLAGS"),  MU_OPTION_DEFAULT,
    N_("enable debugging"),
    mu_c_string, 0, set_debug },
  MU_OPTION_END
};

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
  
  mh_getopt (&argc, &argv, options, 0, NULL, prog_doc, extra_doc);
  if (!append_folder)
    append_folder = mh_global_profile_get ("Inbox", "inbox");
  if (argc)
    {
      mu_error (_("unrecognized arguments"));
      exit (1);
    }

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
      rc = mu_script_init (incdat.handler, script_file, script_env,
			   &incdat.descr);
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


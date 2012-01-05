/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007-2012 Free Software Foundation, Inc.

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

/* MH annotate command */

#include <mh.h>

static char doc[] = N_("GNU MH anno")"\v"
N_("Options marked with `*' are not yet implemented.\n\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[MSGLIST]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("specify folder to operate upon")},
  {"inplace", ARG_INPLACE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* annotate the message in place")},
  {"noinplace", ARG_NOINPLACE, NULL, OPTION_HIDDEN,  "" },
  {"date", ARG_DATE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("add FIELD: date header") },
  {"nodate", ARG_NODATE, NULL, OPTION_HIDDEN, "" },
  {"component", ARG_COMPONENT, N_("FIELD"), 0,
   N_("add this FIELD to the message header") },
  {"text", ARG_TEXT, N_("STRING"), 0,
   N_("field value for the component") },
  { NULL }
};

struct mh_option mh_option[] = {
  { "inplace",   MH_OPT_BOOL },
  { "date",      MH_OPT_BOOL },
  { "component", MH_OPT_ARG, "field" },
  { "text",      MH_OPT_ARG, "body" },
  { NULL }
};

static int inplace;       /* Annotate the message in place */
static int anno_date = 1; /* Add date to the annotation */
static char *component;   /* header field */
static char *anno_text;   /* header field value */

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case ARG_INPLACE:
      mh_opt_notimpl_warning ("-inplace");
      inplace = is_true (arg);
      break;

    case ARG_NOINPLACE:
      mh_opt_notimpl_warning ("-noinplace");
      inplace = 0;
      break;

    case ARG_DATE:
      anno_date = is_true (arg);
      break;

    case ARG_NODATE:
      anno_date = 0;
      break;

    case ARG_COMPONENT:
      component = arg;
      break;

    case ARG_TEXT:
      mh_quote (arg, &anno_text);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
anno (size_t n, mu_message_t msg, void *call_data)
{
  mh_annotate (msg, component, anno_text, anno_date);
  return 0;
}

int
main (int argc, char **argv)
{
  int rc;
  int index;
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
  size_t len;

  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);

  if (!component)
    {
      size_t size = 0;
      char *p;

      if (isatty (0))
	{
	  mu_printf (_("Component name: "));
	  mu_stream_flush (mu_strout);
	}
      rc = mu_stream_getline (mu_strin, &component, &size, NULL);
      if (rc)
	{
	  mu_error (_("error reading input stream: %s"), mu_strerror (rc));
	  exit (1);
	}
      p = mu_str_stripws (component);
      if (*p == 0)
	{
	  mu_error (_("invalid component name"));
	  exit (1);
	}
      if (p > component)
	memmove (component, p, strlen (p) + 1);
    }

  if (!anno_text && !anno_date)
    exit (0);

  len = strlen (component);
  if (len > 0 && component[len-1] == ':')
    component[len-1] = 0;
  
  argc -= index;
  argv += index;
  
  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
  rc = mu_msgset_foreach_message (msgset, anno, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_foreach_message", NULL, rc);
      exit (1);
    }
      
  mh_msgset_first_current (mbox, msgset);
  mu_msgset_free (msgset);
  mh_global_save_state ();
  mu_mailbox_sync (mbox);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return rc;
}
      

				  
  




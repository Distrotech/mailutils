/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

/* MH annotate command */

#include <mh.h>

const char *argp_program_version = "anno (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH anno\v"
"Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[msg [msg...]]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify folder to operate upon")},
  {"inplace", ARG_INPLACE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Annotate the message in place")},
  {"noinplace", ARG_NOINPLACE, NULL, OPTION_HIDDEN,  "" },
  {"date", ARG_DATE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Add FIELD: Date header") },
  {"nodate", ARG_NODATE, NULL, OPTION_HIDDEN, "" },
  {"component", ARG_COMPONENT, N_("FIELD"), 0,
   N_("Add this FIELD to the message header") },
  {"text", ARG_TEXT, N_("STRING"), 0,
   N_("Field value for the component") },
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
  { NULL }
};

struct mh_option mh_option[] = {
  {"inplace",   1, MH_OPT_BOOL },
  {"date",      1, MH_OPT_BOOL },
  {"component", 1, MH_OPT_ARG, "field"},
  {"text",      1, MH_OPT_ARG, "body"},
  { NULL }
};

static int inplace;       /* Annotate the message in place */
static int anno_date = 1; /* Add date to the annotation */
static char *component;   /* header field */
static char *anno_text;   /* header field value */

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      current_folder = arg;
      break;

    case ARG_INPLACE:
      inplace = is_true (arg);
      break;

    case ARG_NOINPLACE:
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
      anno_text = arg;
      break;

    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
    }
  return 0;
}

void
anno (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  mh_annotate (msg, component, anno_text, anno_date);
}

int
main (int argc, char **argv)
{
  int rc;
  int index;
  mailbox_t mbox;
  mh_msgset_t msgset;
  
  mu_init_nls ();

  mh_argp_parse (argc, argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (current_folder, 0);

  if (!component)
    {
      size_t n;
      
      printf (_("Component name: "));
      if (getline (&component, &n, stdin) <= 0 || *component == 0)
	exit (1);
    }

  if (!anno_text && !anno_date)
    exit (0);

  argc -= index;
  argv += index;
  
  mh_msgset_parse (mbox, &msgset, argc, argv, "cur");
  rc = mh_iterate (mbox, &msgset, anno, NULL);

  mailbox_save_attributes (mbox);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return rc;
}
      

				  
  




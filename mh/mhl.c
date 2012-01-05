/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012 Free Software Foundation, Inc.

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

/* MH mhl command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>

static char doc[] = N_("GNU MH mhl")"\v"
N_("Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[FILE [FILE...]]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",     ARG_FOLDER,     N_("FOLDER"), 0,
   N_("specify folder to operate upon")},
  { "bell",      ARG_BELL,       N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("ring the bell at the end of each output page") },
  {"nobell",     ARG_NOBELL,     NULL, OPTION_HIDDEN, "" },
  { "clear",     ARG_CLEAR,      N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("clear the screen after each page of output")},
  {"noclear",    ARG_NOCLEAR,    NULL, OPTION_HIDDEN, "" },
  {"form",       ARG_FORM,       N_("FILE"), 0,
   N_("read format from given file")},
  {"width",      ARG_WIDTH,      N_("NUMBER"), 0,
   N_("set output width")},
  {"length",     ARG_LENGTH,     N_("NUMBER"), 0,
   N_("set output screen length")},
  {"moreproc",   ARG_MOREPROC,   N_("PROG"), 0,
   N_("use given PROG instead of the default") },
  {"nomoreproc", ARG_NOMOREPROC, NULL, 0,
   N_("disable use of moreproc program") },
  { NULL }
};
   
/* Traditional MH options */
struct mh_option mh_option[] = {
  { "bell",       MH_OPT_BOOL },
  { "clear",      MH_OPT_BOOL },
  { "form",       MH_OPT_ARG, "formatfile"},
  { "width",      MH_OPT_ARG, "number"},
  { "length",     MH_OPT_ARG, "number"},
  { "moreproc",   MH_OPT_ARG, "program"},
  { "nomoreproc" },
  { NULL }
};

static int interactive;  /* Using interactive output */
static int mhl_fmt_flags; /* MHL format flags. Controlled by --bell 
                             and --clear */
static int length = 40;  /* Length of output page */
static int width = 80;   /* Width of output page */
static char *formfile = MHLIBDIR "/mhl.format";
static const char *moreproc;
static int nomoreproc;

static mu_list_t format;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER:
      mh_set_current_folder (arg);
      break;

    case ARG_BELL:
      if (is_true (arg))
        mhl_fmt_flags |= MHL_BELL;
      break;
      
    case ARG_NOBELL:
      mhl_fmt_flags &= ~MHL_BELL;
      break;
      
    case ARG_CLEAR:
      if (is_true (arg))
        mhl_fmt_flags |= MHL_CLEARSCREEN;
      break;
      
    case ARG_NOCLEAR:
      mhl_fmt_flags &= ~MHL_CLEARSCREEN;
      break;
      
    case ARG_FORM:
      mh_find_file (arg, &formfile);
      break;
      
    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("invalid width"));
	  exit (1);
	}
      break;
      
    case ARG_LENGTH:
      length = strtoul (arg, NULL, 0);
      if (!length)
	{
	  argp_error (state, _("invalid length"));
	  exit (1);
	}
      break;

    case ARG_MOREPROC:
      moreproc = arg;
      break;
      
    case ARG_NOMOREPROC:
      nomoreproc = 1;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static mu_stream_t
open_output ()
{
  int rc;
  mu_stream_t output;

  if (interactive && !nomoreproc)
    {
      if (!moreproc || !moreproc[0])
	moreproc = mh_global_profile_get ("moreproc", getenv ("PAGER"));
    }
  else
    moreproc = NULL;

  if (moreproc)
    rc = mu_command_stream_create (&output, moreproc, MU_STREAM_WRITE);
  else
    {
      rc = 0;
      output = mu_strout;
      mu_stream_ref (output);
    }

  if (rc)
    {
      mu_error (_("cannot create output stream: %s"), mu_strerror (rc));
      exit (1);
    }

  return output;
}

static void
list_message (char *name, mu_stream_t output)
{
  int rc;
  mu_stream_t input;
  mu_message_t msg;

  if (!name)
    {
      rc = 0;
      input = mu_strin;
      mu_stream_ref (input);
    }
  else
    rc = mu_file_stream_create (&input, name, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("cannot create input stream: %s"), mu_strerror (rc));
      return;
    }

  msg = mh_stream_to_message (input);
  if (!msg)
    {
      mu_error (_("input stream %s is not a message (%s)"),
		name, mu_strerror (rc));
      mu_stream_close (input);
      mu_stream_destroy (&input);
    }
  else
    {
      mhl_format_run (format, width, length, mhl_fmt_flags, msg, output);
      mu_message_unref (msg);
    }
}

int
main (int argc, char **argv)
{
  int index;
  mu_stream_t output;
  
  interactive = isatty (1) && isatty (0);
  
  MU_APP_INIT_NLS ();
  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  format = mhl_format_compile (formfile);
  if (!format)
    exit (1);
  
  argc -= index;
  argv += index;

  if (argc == 0)
    nomoreproc = 1;

  if (!interactive)
    mhl_fmt_flags &= ~MHL_BELL;
  
  output = open_output ();
  
  if (argc == 0)
    list_message (NULL, output);
  else
    while (argc--)
      list_message (*argv++, output);

  mu_stream_close (output);
  return 0;
}

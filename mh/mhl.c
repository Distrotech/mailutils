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

/* MH mhl command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>

const char *program_version = "mhl (" PACKAGE_STRING ")";
/* TRANSLATORS: Please, preserve the vertical tabulation (^K character)
   in this message */
static char doc[] = N_("GNU MH mhl\v\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[files]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",     ARG_FOLDER,     N_("FOLDER"), 0,
   N_("Specify folder to operate upon")},
  { "bell",      ARG_BELL,       N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("Ring the bell at the end of each output page") },
  {"nobell",     ARG_NOBELL,     NULL, OPTION_HIDDEN, "" },
  { "clear",     ARG_CLEAR,      N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("Clear the screen after each page of output")},
  {"noclear",    ARG_NOCLEAR,    NULL, OPTION_HIDDEN, "" },
  {"form",       ARG_FORM,       N_("FILE"), 0,
   N_("Read format from given file")},
  {"width",      ARG_WIDTH,      N_("NUMBER"), 0,
   N_("Set output width")},
  {"length",     ARG_LENGTH,     N_("NUMBER"), 0,
   N_("Set output screen length")},
  {"moreproc",   ARG_MOREPROC,   N_("PROG"), 0,
   N_("Use given PROG instead of the default") },
  {"nomoreproc", ARG_NOMOREPROC, NULL, 0,
   N_("Disable use of moreproc program") },
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
  { NULL }
};
   
/* Traditional MH options */
struct mh_option mh_option[] = {
  { "bell",       1,  MH_OPT_BOOL },
  { "clear",      1,  MH_OPT_BOOL },
  { "form",       1,  MH_OPT_ARG, "formatfile"},
  { "width",      1,  MH_OPT_ARG, "number"},
  { "length",     1,  MH_OPT_ARG, "number"},
  { "moreproc",   1,  MH_OPT_ARG, "program"},
  { "nomoreproc", 3, },
  { NULL }
};

static int interactive;  /* Using interactive output */
static int mhl_fmt_flags; /* MHL format flags. Controlled by --bell 
                             and --clear */
static int length = 40;  /* Length of output page */
static int width = 80;   /* Width of output page */
static char *formfile = MHLIBDIR "/mhl.format";
static char *moreproc;
static int nomoreproc;

static list_t format;

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER:
      current_folder = arg;
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
      formfile = arg;
      break;
      
    case ARG_WIDTH:
      width = strtoul (arg, NULL, 0);
      if (!width)
	{
	  argp_error (state, _("Invalid width"));
	  exit (1);
	}
      break;
      
    case ARG_LENGTH:
      length = strtoul (arg, NULL, 0);
      if (!length)
	{
	  argp_error (state, _("Invalid length"));
	  exit (1);
	}
      break;

    case ARG_MOREPROC:
      moreproc = arg;
      break;
      
    case ARG_NOMOREPROC:
      nomoreproc = 1;
      break;
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
    }
  return 0;
}

static stream_t
open_output ()
{
  int rc;
  stream_t output;

  if (interactive && !nomoreproc)
    {
      if (!moreproc)
	moreproc = mh_global_profile_get ("moreproc", getenv ("PAGER"));
    }
  else
    moreproc = NULL;

  if (moreproc)
    rc = prog_stream_create (&output, moreproc, MU_STREAM_WRITE);
  else
    rc = stdio_stream_create (&output, stdout, MU_STREAM_WRITE);

  if (rc)
    {
      mh_error (_("cannot create output stream: %s"), mu_strerror (rc));
      exit (1);
    }

  if ((rc = stream_open (output)))
    {
      mh_error (_("cannot open output stream: %s"), mu_strerror (rc));
      exit (1);
    }
  return output;
}

static void
list_message (char *name, stream_t output)
{
  int rc;
  stream_t input;
  message_t msg;

  if (!name)
    rc = stdio_stream_create (&input, stdin, MU_STREAM_SEEKABLE);
  else
    rc = file_stream_create (&input, name, MU_STREAM_READ);
  if (rc)
    {
      mh_error (_("cannot create input stream: %s"), mu_strerror (rc));
      return;
    }

  if ((rc = stream_open (input)))
    {
      mh_error (_("cannot open input stream: %s"), mu_strerror (rc));
      stream_destroy (&input, stream_get_owner (input));
      return;
    }

  msg = mh_stream_to_message (input);
  if (!msg)
    {
      mh_error (_("input stream %s is not a message (%s)"),
		name, mu_strerror (rc));
      stream_close (input);
      stream_destroy (&input, stream_get_owner (input));
    }
  else
    {
      mhl_format_run (format, width, length, mhl_fmt_flags, msg, output);
      message_unref (msg);
    }
}

int
main (int argc, char **argv)
{
  int index;
  stream_t output;
  
  interactive = isatty (1) && isatty (0);
  
  mu_init_nls ();
  mu_argp_init (program_version, NULL);
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

  stream_close (output);
  return 0;
}

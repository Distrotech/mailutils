/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012, 2014-2016 Free Software Foundation,
   Inc.

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

static char prog_doc[] = N_("GNU MH mhl");
static char args_doc[] = N_("[FILE [FILE...]]");

static int bell_option;
static int clear_option;

static int interactive;  /* Using interactive output */
static int mhl_fmt_flags; /* MHL format flags. Controlled by -bell 
                             and -clear */
static int length = 40;  /* Length of output page */
static int width = 80;   /* Width of output page */
static char *formfile = MHLIBDIR "/mhl.format";
static const char *moreproc;
static int nomoreproc;

static mu_list_t format;

static struct mu_option options[] = {
  { "bell",      0,       NULL, MU_OPTION_DEFAULT,
    N_("ring the bell at the end of each output page"),
    mu_c_bool, &bell_option },
  { "clear",     0,       NULL, MU_OPTION_DEFAULT,
    N_("clear the screen after each page of output"),
    mu_c_bool, &clear_option },
  { "form",      0,       N_("FILE"), MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, &formfile, mh_opt_find_file },
  { "width",     0,       N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output width"),
    mu_c_int, &width },
  { "length",    0,       N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output screen length"),
    mu_c_int, &length },
  { "moreproc",  0,       N_("PROG"), MU_OPTION_DEFAULT,
    N_("use given PROG instead of the default"),
    mu_c_string, &moreproc },
  { "nomoreproc", 0, NULL, MU_OPTION_DEFAULT,
    N_("disable use of moreproc program"),
    mu_c_bool, &nomoreproc },
  MU_OPTION_END
};
   
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
  mu_stream_t output;
  
  interactive = isatty (1) && isatty (0);
  
  MU_APP_INIT_NLS ();

  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);  

  if (bell_option == -1)
    /* use default */;
  else if (bell_option)
    mhl_fmt_flags |= MHL_BELL;
  else
    mhl_fmt_flags &= ~MHL_BELL;

  if (clear_option == -1)
    /* use default */;
  else if (clear_option)
    mhl_fmt_flags |= MHL_CLEARSCREEN;
  else
    mhl_fmt_flags &= ~MHL_CLEARSCREEN;
  
  format = mhl_format_compile (formfile);
  if (!format)
    exit (1);
  
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

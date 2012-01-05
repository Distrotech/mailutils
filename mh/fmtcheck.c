/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2012 Free Software Foundation, Inc.

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

/* fmtcheck */

#include <mh.h>

static char doc[] = N_("GNU MH fmtcheck")"\v"
N_("Use -help to obtain the list of traditional MH options.");

/* GNU options */
static struct argp_option options[] = {
  {"form",    ARG_FORM, N_("FILE"),   0,
   N_("read format from given file")},
  {"format",  ARG_FORMAT, N_("FORMAT"), 0,
   N_("use this format string")},
  {"dump",    ARG_DUMP, NULL,     0,
   N_("dump the listing of compiled format code")},
  { "debug",  ARG_DEBUG, NULL,     0,
    N_("enable parser debugging output"),},

  { NULL }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "form",    MH_OPT_ARG, "formatfile" },
  { "format",  MH_OPT_ARG, "string" },
  { NULL }
};

char *format_str;
static mh_format_t format;

typedef int (*action_fp) (void);

static int
action_dump ()
{
  if (!format_str)
    {
      mu_error (_("Format string not specified"));
      return 1;
    }
  mh_format_dump (&format);
  return 0;
}

static action_fp action = action_dump;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FORM:
      mh_read_formfile (arg, &format_str);
      break;

    case ARG_FORMAT:
      format_str = arg;
      break;

    case ARG_DUMP:
      action = action_dump;
      break;

    case ARG_DEBUG:
      mh_format_debug (1);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, NULL, doc,
		 opt_handler, NULL, NULL);

  if (format_str && mh_format_parse (format_str, &format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }
  return (*action) ();
}

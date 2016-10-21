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

/* fmtcheck */

#include <mh.h>

static char prog_doc[] = N_("GNU MH fmtcheck");

char *format_str;
static mh_format_t format;
int dump_option;
int debug_option;

static struct mu_option options[] = {
  { "form",    0, N_("FILE"),   MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, &format_str, mh_opt_read_formfile },
  
  { "format",  0, N_("FORMAT"), MU_OPTION_DEFAULT,
    N_("use this format string"),
    mu_c_string, &format_str },
  { "dump",    0, NULL,     MU_OPTION_HIDDEN,
    N_("dump the listing of compiled format code"),
    mu_c_bool,   &dump_option },
  { "debug",   0, NULL,     MU_OPTION_DEFAULT,
    N_("enable parser debugging output"),
    mu_c_bool,   &debug_option },

  MU_OPTION_END
};

static int
action_dump (void)
{
  if (!format_str)
    {
      mu_error (_("Format string not specified"));
      return 1;
    }
  mh_format_dump (&format);
  return 0;
}

int
main (int argc, char **argv)
{
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_getopt (&argc, &argv, options, 0, NULL, prog_doc, NULL);
  mh_format_debug (debug_option);
  if (format_str && mh_format_parse (format_str, &format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }
  return action_dump ();
}

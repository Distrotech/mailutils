/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* fmtcheck */

#include <mh.h>

const char *argp_program_version = "fmtcheck (" PACKAGE_STRING ")";
static char doc[] = "GNU MH fmtcheck";
static char args_doc[] = "";

/* GNU options */
static struct argp_option options[] = {
  {"form",    'F', N_("FILE"),   0, N_("Read format from given file")},
  {"format",  't', N_("FORMAT"), 0, N_("Use this format string")},
  {"dump",    'd', NULL,     0, N_("Dump the listing of compiled format code")},
  { N_("\nUse -help switch to obtain the list of traditional MH options. "), 0, 0, OPTION_DOC, "" },
  
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"form",    4,  'F', MH_OPT_ARG, "formatfile"},
  {"format",  5,  't', MH_OPT_ARG, "string"},
  { 0 }
};

char *format_str;
static mh_format_t format;

typedef int (*action_fp) __P((void));

static int
action_dump ()
{
  if (!format_str)
    {
      mh_error (_("format string not specified"));
      return 1;
    }
  mh_format_dump (&format);
  return 0;
}

static action_fp action = action_dump;

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case 'F':
      mh_read_formfile (arg, &format_str);
      break;

    case 't':
      format_str = arg;
      break;

    case 'd':
      action = action_dump;
      break;
      
    default:
      return 1;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);

  if (format_str && mh_format_parse (format_str, &format))
    {
      mh_error (_("Bad format string"));
      exit (1);
    }
  return (*action) ();
}

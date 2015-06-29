/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012, 2014-2015 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "cmdline.h"
#include <unistd.h>
#include <stdlib.h>


/* ************************************************************************* */
/* GNU Mailutils help and version output                                     */
/* ************************************************************************* */

#define OPT_PROGNAME    -2
#define OPT_USAGE       -3
#define OPT_HANG        -4

static struct argp_option mu_mailutils_argp_options[] = 
{
  {"help",        '?',          0, 0,  N_("give this help list"), -1},
  {"usage",       OPT_USAGE,    0, 0,  N_("give a short usage message"), 0},
  {"version",     'V',          0, 0,  N_("print program version"), -1},
  {"program-name",OPT_PROGNAME,N_("NAME"), OPTION_HIDDEN, N_("set the program name"), 0},
  {"HANG",        OPT_HANG,    N_("SECS"), OPTION_ARG_OPTIONAL | OPTION_HIDDEN,
     N_("hang for SECS seconds (default 3600)"), 0},
  { NULL }
};

static error_t
mu_mailutils_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case '?':
      argp_state_help (state, state->out_stream,
		       ARGP_HELP_SHORT_USAGE | ARGP_HELP_LONG | ARGP_HELP_DOC);
      /* TRANSLATORS: The placeholder indicates the bug-reporting address
	 for this package.  Please add _another line_ saying
	 "Report translation bugs to <...>\n" with the address for translation
	 bugs (typically your translation team's web or email address).  */
      printf (_("\nReport bugs to: %s\n"), "<" PACKAGE_BUGREPORT ">");
      
#ifdef PACKAGE_PACKAGER_BUG_REPORTS
      printf (_("Report %s bugs to: %s\n"), PACKAGE_PACKAGER,
	      PACKAGE_PACKAGER_BUG_REPORTS);
#endif
      
#ifdef PACKAGE_URL
      printf (_("%s home page: <%s>\n"), PACKAGE_NAME, PACKAGE_URL);
#endif
      fputs (_("General help using GNU software: <http://www.gnu.org/gethelp/>\n"),
	stdout);
      exit (0);

    case OPT_USAGE:
      argp_state_help (state, state->out_stream,
		       ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK);
      break;

    case 'V':
      if (argp_program_version_hook)
        (*argp_program_version_hook) (state->out_stream, state);
      else if (argp_program_version)
        fprintf (state->out_stream, "%s\n", argp_program_version);
      else
        argp_error (state, "%s",
		    dgettext (state->root_argp->argp_domain,
			      "(PROGRAM ERROR) No version known!?"));
      exit(0);
      
    case OPT_PROGNAME:          /* Set the program name.  */
#if HAVE_DECL_PROGRAM_INVOCATION_NAME
      program_invocation_name = arg;
#endif
      /* Update what we use for messages.  */
      state->name = __argp_base_name (arg);

#if HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME
      program_invocation_short_name = state->name;
#endif

      if ((state->flags & (ARGP_PARSE_ARGV0 | ARGP_NO_ERRS))
	  == ARGP_PARSE_ARGV0)
        /* Update what getopt uses too.  */
        state->argv[0] = arg;
      break;

    case OPT_HANG:
      {
	int hang = atoi (arg ? arg : "3600");
	while (hang-- > 0)
	  sleep (1);
	break;      
      }
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_mailutils_argp = {
  mu_mailutils_argp_options,
  mu_mailutils_argp_parser,
};

struct argp_child mu_mailutils_argp_child = {
  &mu_mailutils_argp,
  0,
  NULL,
  -1,
};

static void
mu_mailutils_modflags(int *flags)
{
  *flags |= ARGP_NO_HELP;
}

struct mu_cmdline_capa mu_mailutils_cmdline = {
  "mailutils", &mu_mailutils_argp_child, mu_mailutils_modflags
};


    


  

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mh.h"
#include <mailutils/stream.h>
#include <mailutils/wordsplit.h>
#include <mailutils/io.h>
#include <mailutils/cli.h>

struct getopt_data
{
  char *extra_doc;
};

static void
mh_extra_help_hook (struct mu_parseopt *po, mu_stream_t stream)
{
  struct getopt_data *data = po->po_data;
  mu_stream_printf (stream, "%s\n", _(data->extra_doc));
}

static void
augment_argv (int *pargc, char ***pargv)
{
  int argc;
  char **argv;
  int i, j;
  struct mu_wordsplit ws;
  char const *val = mh_global_profile_get (mu_program_name, NULL);

  if (!val)
    return;
      
  if (mu_wordsplit (val, &ws, MU_WRDSF_DEFFLAGS))
    {
      mu_error (_("cannot split line `%s': %s"), val,
		mu_wordsplit_strerror (&ws));
      exit (1);
    }

  argc = *pargc + ws.ws_wordc;
  argv = calloc (argc + 1, sizeof *argv);
  if (!argv)
    mh_err_memory (1);

  i = 0;
  argv[i++] = (*pargv)[0];
  for (j = 0; j < ws.ws_wordc; i++, j++)
    argv[i] = ws.ws_wordv[j];
  for (j = 1; i < argc; i++, j++)
    argv[i] = (*pargv)[j];
  argv[i] = NULL;
  
  ws.ws_wordc = 0;
  mu_wordsplit_free (&ws);

  *pargc = argc;
  *pargv = argv;
}

static void
process_std_options (int argc, char **argv, struct mu_parseopt *po)
{
  if (argc != 1)
    return;
  if (strcmp (argv[0], "--help") == 0)
    {
      mu_program_help (po, mu_strout);
      exit (0);
    }
  if (strcmp (argv[0], "--version") == 0)
    {
      mu_program_version (po, mu_strout);
      exit (0);
    }
}
  
static void
process_folder_arg (int *pargc, char **argv, struct mu_parseopt *po)
{
  int i, j;
  int argc = *pargc;
  struct mu_option *opt;
  
  /* Find folder option */
  for (i = 0; ; i++)
    {
      if (!po->po_optv[i])
	return; /* Nothing to do */
      if (MU_OPTION_IS_VALID_LONG_OPTION (po->po_optv[i])
	  && strcmp (po->po_optv[i]->opt_long, "folder") == 0)
	break;
    }
  opt = po->po_optv[i];
    
  for (i = j = 0; i < argc; i++)
    {
      if (argv[i][0] == '+')
	{
	  opt->opt_set (po, opt, argv[i] + 1);
	}
      else
	argv[j++] = argv[i];
    }
  argv[j] = NULL;
  *pargc = j;
}

void
mh_opt_set_folder (struct mu_parseopt *po, struct mu_option *opt,
		   char const *arg)
{
  mh_set_current_folder (arg);
}

static struct mu_option folder_option[] = {
  { "folder", 0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("set current folder"),
    mu_c_string, NULL, mh_opt_set_folder },
  MU_OPTION_END
};

void
mh_version_hook (struct mu_parseopt *po, mu_stream_t stream)
{
#ifdef GIT_DESCRIBE
  mu_stream_printf (stream, "%s (%s %s) [%s]\n",
		    mu_program_name, PACKAGE_NAME, PACKAGE_VERSION,
		    GIT_DESCRIBE);
#else
  mu_stream_printf (stream, "%s (%s %s)\n", mu_program_name,
		    PACKAGE_NAME, PACKAGE_VERSION);
#endif
  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  mu_stream_printf (stream, mu_version_copyright, _("(C)"));
  mu_stream_printf (stream, _("\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
"));
}

static int
has_folder_option (struct mu_option *opt)
{
  while (!MU_OPTION_IS_END (opt))
    {
      if (MU_OPTION_IS_VALID_LONG_OPTION (opt)
	  && strcmp (opt->opt_long, "folder") == 0)
	return 1;
      ++opt;
    }
  return 0;
}

void
mh_getopt (int *pargc, char ***pargv, struct mu_option *options,
	   int mhflags,
	   char *argdoc, char *progdoc, char *extradoc)
{
  int argc = *pargc;
  char **argv = *pargv;
  struct mu_parseopt po;
  struct mu_option *optv[3];
  struct getopt_data getopt_data;
  char const *args[3];
  int flags = MU_PARSEOPT_SINGLE_DASH | MU_PARSEOPT_IMMEDIATE;
  int i;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  po.po_negation = "no";
  flags |= MU_PARSEOPT_NEGATION;

  if ((mhflags & MH_GETOPT_DEFAULT_FOLDER) || has_folder_option (options))
    {
      po.po_special_args = N_("[+FOLDER]");
      flags |= MU_PARSEOPT_SPECIAL_ARGS;
    }

  if (argdoc)
    {
      args[0] = argdoc;
      args[1] = NULL;
      po.po_prog_args = args;
      flags |= MU_PARSEOPT_PROG_ARGS;
    }
  if (progdoc)
    {
      po.po_prog_doc = progdoc;
      flags |= MU_PARSEOPT_PROG_DOC;
    }

  getopt_data.extra_doc = extradoc;
  if (extradoc)
    {
      po.po_help_hook = mh_extra_help_hook;
      flags |= MU_PARSEOPT_HELP_HOOK;
    }

  po.po_data = &getopt_data;
  flags |= MU_PARSEOPT_DATA;
  
  po.po_exit_error = 1;
  flags |= MU_PARSEOPT_EXIT_ERROR;
  
  po.po_package_name = PACKAGE_NAME;
  flags |= MU_PARSEOPT_PACKAGE_NAME;

  po.po_package_url = PACKAGE_URL;
  flags |= MU_PARSEOPT_PACKAGE_URL;

  po.po_bug_address = PACKAGE_BUGREPORT;
  flags |= MU_PARSEOPT_BUG_ADDRESS;

  //po.po_extra_info = gnu_general_help_url;
  //flags |= MU_PARSEOPT_EXTRA_INFO;

  po.po_version_hook = mh_version_hook;
  flags |= MU_PARSEOPT_VERSION_HOOK;
  
  mu_set_program_name (argv[0]);
  mh_init ();
  augment_argv (&argc, &argv);

  i = 0;
  if (mhflags & MH_GETOPT_DEFAULT_FOLDER)
    optv[i++] = folder_option;
  if (options)
    optv[i++] = options;
  optv[i] = NULL;
  
  if (mu_parseopt (&po, argc, argv, optv, flags))
    exit (po.po_exit_error);

  argc -= po.po_arg_start;
  argv += po.po_arg_start;

  process_std_options (argc, argv, &po);
  
  process_folder_arg (&argc, argv, &po);

  if (!argdoc && argc)
    {
      mu_diag_init ();
      mu_stream_printf (mu_strerr, "\033s<%d>", MU_DIAG_ERROR);
      mu_stream_printf (mu_strerr, "%s", _("unrecognized extra arguments:"));
      for (i = 0; i < argc; i++)
	mu_stream_printf (mu_strerr, " %s", argv[i]);
      mu_stream_write (mu_strerr, "\n", 1, NULL);
      exit (1);
    }

  *pargc = argc;
  *pargv = argv;
  
  mh_init2 ();
}

void
mh_opt_notimpl (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mu_error (_("option is not yet implemented: %s"), opt->opt_long);
  exit (1);
}

void
mh_opt_notimpl_warning (struct mu_parseopt *po, struct mu_option *opt,
			char const *arg)
{
  if (opt->opt_type == mu_c_bool)
    {
      int val;
      if (mu_str_to_c (arg, opt->opt_type, &val, NULL) == 0 && !val)
	return;
    }
  mu_error (_("ignoring not implemented option %s"), opt->opt_long);
}

void
mh_opt_clear_string (struct mu_parseopt *po, struct mu_option *opt,
		     char const *arg)
{
  char **sptr = opt->opt_ptr;
  free (*sptr);
  *sptr = NULL;
}

void
mh_opt_find_file (struct mu_parseopt *po, struct mu_option *opt,
		  char const *arg)
{
  mh_find_file (arg, opt->opt_ptr);
}

void
mh_opt_read_formfile (struct mu_parseopt *po, struct mu_option *opt,
		      char const *arg)
{
  mh_read_formfile (arg, opt->opt_ptr);
}

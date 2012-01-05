/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2007, 2009-2012 Free Software
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

/* Coexistence between GNU long options, traditional UNIX-style short
   options and traditional MH long options. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mh.h>
#include <string.h>
#include <mailutils/argcv.h>
#include "argp.h"
#ifdef MU_ALPHA_RELEASE
# include <git-describe.h>
#endif

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct mh_argp_data *data = state->input;
  error_t ret = ARGP_ERR_UNKNOWN;
  
  switch (key)
    {
    case ARGP_KEY_ARG:
      if (arg[0] == '+')
	ret = data->handler (ARG_FOLDER, arg + 1, state);
      break;
      
    default:
      ret = data->handler (key, arg, state);
      if (ret == 0)
	{
	  if (key == ARGP_KEY_ERROR)
	    data->errind = state->next;
	}
    }

  return ret;
}

static int
my_argp_parse (struct argp *argp, int argc, char **argv, int flags,
	       int *end_index, struct mh_argp_data *data)
{
  int rc;
  int f = 0;
  int index = 0;

  if (flags & ARGP_NO_ERRS)
    {
      while (argc > 0
	     && (rc = argp_parse (argp, argc, argv, flags|f, end_index, data))
	     == EINVAL)
	{
	  if (data->errind == -1)
	    break;
	  data->errind--;
	  if (f)
	    data->errind--;
	  argc -= data->errind;
	  argv += data->errind;
	  index += data->errind;
	  if (argc < 2 || memcmp (argv[1], "--", 2))
	    {
	      if (end_index)
		*end_index = index + 1;
	      break;
	    }
	  f = ARGP_PARSE_ARGV0;
	}
      if (rc == 0 && end_index)
	*end_index += index;
      rc = 0;
    }
  else
    rc = argp_parse (argp, argc, argv, flags, end_index, data);
  return rc;
}

const char version_etc_copyright[] =
  /* Do *not* mark this string for translation.  %s is a copyright
     symbol suitable for this locale, and %d is the copyright
     year.  */
  "Copyright %s 2010 Free Software Foundation, inc.";

/* This is almost the same as mu_program_version_hook from muinit.c,
   except for different formatting of the first line. MH uses:

      progname (GNU Mailutils X.Y.Z)

   where X.Y.Z stands for the version number. Emacs MH-E uses this
   to determine Mailutils presence and its version number (see
   lisp/mh-e/mh-e.el, function mh-variant-mu-mh-info). */
static void
mh_program_version_hook (FILE *stream, struct argp_state *state)
{
#ifdef GIT_DESCRIBE
  fprintf (stream, "%s (%s %s) [%s]\n",
	   mu_program_name, PACKAGE_NAME, PACKAGE_VERSION, GIT_DESCRIBE);
#else
  fprintf (stream, "%s (%s %s)\n", mu_program_name,
	   PACKAGE_NAME, PACKAGE_VERSION);
#endif
  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  fprintf (stream, version_etc_copyright, _("(C)"));

  fputs (_("\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
"),
	 stream);
}

void
mh_argp_init ()
{
  argp_program_bug_address =  "<" PACKAGE_BUGREPORT ">";
  argp_program_version_hook = mh_program_version_hook;
}


enum
  {
    OPT_DEBUG_LEVEL = 256,
    OPT_DEBUG_LINE_INFO,
  };

static struct argp_option mu_debug_argp_options[] = 
{
  { "debug-level", OPT_DEBUG_LEVEL, N_("LEVEL"), 0,
    N_("set Mailutils debugging level"), 0 },
  { "debug-line-info", OPT_DEBUG_LINE_INFO, NULL, 0,
    N_("show source info with debugging messages"), 0 },
  { NULL }
};

static error_t
mu_debug_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case OPT_DEBUG_LEVEL:
      mu_debug_parse_spec (arg);
      break;

    case OPT_DEBUG_LINE_INFO:
      mu_debug_line_info = 1;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_debug_argp = {
  mu_debug_argp_options,
  mu_debug_argp_parser,
};

struct argp_child mh_argp_children[] = {
  { &mu_debug_argp, 0, N_("Global debugging settings"), -2 },
  { NULL }
};

int
mh_argp_parse (int *pargc, char **pargv[],
	       int flags,
	       struct argp_option *option,
	       struct mh_option *mh_option,
	       char *argp_doc, char *doc,
	       argp_parser_t handler,
	       void *closure, int *pindex)
{
  struct argp argp;
  struct mh_argp_data data;
  const char *val;
  int index;
  int extra  = 0;

  mu_set_program_name ((*pargv)[0]);
  mh_init ();

  mh_option_init (mh_option);
  memset (&argp, 0, sizeof (argp));
  argp.options = option;
  argp.parser = parse_opt;
  argp.args_doc = argp_doc;
  argp.doc = doc;
  argp.children = mh_argp_children;
  data.mh_option = mh_option;
  data.closure = closure;
  data.handler = handler;
  data.doc = argp_doc;
  data.errind = -1;
  
  val = mh_global_profile_get (mu_program_name, NULL);
  if (val)
    {
      int argc;
      char **argv;
      int i, j;
      struct mu_wordsplit ws;
      
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
      
      mh_argv_preproc (argc, argv, &data);

      my_argp_parse (&argp, argc, argv, flags, &index, &data);

      extra = index < argc;

      *pargc = argc;
      *pargv = argv;
    }
  else
    {
      mh_argv_preproc (*pargc, *pargv, &data);
      my_argp_parse (&argp, *pargc, *pargv, flags, &index, &data);
      extra = index < *pargc;
    }
  if (pindex)
    *pindex = index;
  else if (extra)
    {
      mu_error (_("Extra arguments"));
      exit (1);
    }
  mh_init2 ();
  return 0;
}

void
mh_license (const char *name)
{
  printf (_("This is %s\n\n"), name);
  printf (
  _("   GNU Mailutils is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 3 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.\n"
    "\n"
    "\n"
));

  exit (0);
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

/* Coexistence between GNU long options, traditional UNIX-style short
   options and traditional MH long options. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mh.h>
#include <string.h>
#include <mailutils/argcv.h>

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct mh_argp_data *data = state->input;
  switch (key)
    {
    case ARGP_KEY_ARG:
      if (arg[0] == '+')
	{
	  data->handler (ARG_FOLDER, arg + 1, data->closure);
	  break;
	}
      return ARGP_ERR_UNKNOWN;

    default:
      if (data->handler (key, arg, data->closure, state) == 0)
	{
	  if (key == ARGP_KEY_ERROR)
	    data->errind = state->next;
	  break;
	}
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
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

int
mh_argp_parse (int *pargc, char **pargv[],
	       int flags,
	       struct argp_option *option,
	       struct mh_option *mh_option,
	       char *argp_doc, char *doc,
	       int (*handler)(), void *closure, int *pindex)
{
  struct argp argp;
  struct mh_argp_data data;
  char *p;
  int index;
  int extra  = 0;
  
  program_invocation_name = (*pargv)[0];
  p = strrchr ((*pargv)[0], '/');
  if (p)
    program_invocation_short_name = p+1;
  else
    program_invocation_short_name = program_invocation_name;

  mh_init ();
  
  memset (&argp, 0, sizeof (argp));
  argp.options = option;
  argp.parser = parse_opt;
  argp.args_doc = argp_doc;
  argp.doc = doc;
  data.mh_option = mh_option;
  data.closure = closure;
  data.handler = handler;
  data.doc = argp_doc;
  data.errind = -1;
  
  p = mh_global_profile_get (program_invocation_short_name, NULL);
  if (p)
    {
      int argc;
      char **argv;
      int xargc;
      char **xargv;
      int i, j;
      
      mu_argcv_get (p, "", NULL, &xargc, &xargv);

      argc = *pargc + xargc;
      argv = calloc (argc+1, sizeof *argv);
      if (!argv)
        mh_err_memory (1);

      i = 0;
      argv[i++] = (*pargv)[0];
      for (j = 0; j < xargc; i++, j++)
	argv[i] = xargv[j];
      for (j = 1; i < argc; i++, j++)
	argv[i] = (*pargv)[j];
      argv[i] = NULL;
      
      mh_argv_preproc (argc, argv, &data);

      my_argp_parse (&argp, argc, argv, flags, &index, &data);

      extra = index < argc;

      *pargc = argc;
      *pargv = argv;
      free (xargv);
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
      mh_error (_("Extra arguments"));
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
    "   the Free Software Foundation; either version 2 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils; if not, write to the Free Software Foundation,\n"
    "   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
    "\n"
    "\n"
));

  exit (0);
}


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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* Coexistence between GNU long options, traditional UNIX-style short
   options and traditional MH long options. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mh.h>
#include <string.h>
#include <mailutils/argcv.h>

struct mh_argp_data
{
  struct mh_option *mh_option;
  int (*handler)();
  void *closure;
  char *doc;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct mh_argp_data *data = state->input;
  switch (key)
    {
    case ARGP_KEY_INIT:
      while ((key = mh_getopt (state->argc, state->argv, data->mh_option,
			       data->doc))
	     != EOF
	     && key != '?')
	{
	  data->handler (key, mh_optarg, data->closure);
	}
      state->next = mh_optind;
      break;

    case ARGP_KEY_ARG:
      if (arg[0] == '+')
	{
	  data->handler ('+', arg+1, data->closure);
	  break;
	}
      return ARGP_ERR_UNKNOWN;
	
    default:
      if (data->handler (key, arg, data->closure) == 0)
	break;
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
mh_argp_parse (int argc, char **argv,
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
  
  program_invocation_name = argv[0];
  p = strrchr (argv[0], '/');
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
  
  p = mh_global_profile_get (program_invocation_short_name, NULL);
  if (p)
    {
      int _argc;
      char **_argv;
      int xargc;
      char **xargv;
      int i, j;
      
      argcv_get (p, "", NULL, &xargc, &xargv);

      _argc = argc + xargc;
      _argv = calloc (_argc+1, sizeof *_argv);
      if (!_argv)
	{
	  mh_error (_("not enough memory"));
	  abort ();
	}

      i = 0;
      _argv[i++] = argv[0];
      for (j = 0; j < xargc; i++, j++)
	_argv[i] = xargv[j];
      for (j = 1; i < _argc; i++, j++)
	_argv[i] = argv[j];
      _argv[i] = NULL;
      argp_parse (&argp, _argc, _argv, 0, &index, &data);
      free (_argv);
      extra = index < _argc;
      index -= xargc;
      if (index < 0)
	index = argc;
    }
  else
    {
      argp_parse (&argp, argc, argv, 0, &index, &data);
      extra = index < argc;
    }
  if (pindex)
    *pindex = index;
  else if (extra)
    {
      mh_error (_("extra arguments"));
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
    "   You should have received a copy of the GNU General Public License\n"
    "   along with GNU Mailutils; if not, write to the Free Software\n"
    "   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA\n\n"));

  exit (0);
}


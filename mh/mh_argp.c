/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Coexistence between GNU long options, traditional UNIX-style short
   options and traditional MH long options. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mh_getopt.h>
#include <string.h>

struct mh_argp_data
{
  struct mh_option *mh_option;
  int (*handler)();
  void *closure;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct mh_argp_data *data = state->input;
  switch (key)
    {
    case ARGP_KEY_INIT:
      while ((key = mh_getopt (state->argc, state->argv, data->mh_option))
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
	       int (*handler)(), void *closure)
{
  struct argp argp;
  struct mh_argp_data data;
  char *p;
  
  program_invocation_name = argv[0];
  p = strrchr (argv[0], '/');
  if (p)
    program_invocation_short_name = p+1;
  else
    program_invocation_short_name = program_invocation_name;
  memset (&argp, 0, sizeof (argp));
  argp.options = option;
  argp.parser = parse_opt;
  argp.args_doc = argp_doc;
  argp.doc = doc;
  data.mh_option = mh_option;
  data.closure = closure;
  data.handler = handler;
  return argp_parse (&argp, argc, argv, 0, 0, &data);
}

void
mh_license (const char *name)
{
  static char license_text[] =
"   This program is free software; you can redistribute it and/or modify\n"
"   it under the terms of the GNU General Public License as published by\n"
"   the Free Software Foundation; either version 2, or (at your option)\n"
"   any later version.\n"
"\n"
"   This program is distributed in the hope that it will be useful,\n"
"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"   GNU General Public License for more details.\n"
"\n"
"   You should have received a copy of the GNU General Public License\n"
"   along with this program; if not, write to the Free Software\n"
"   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n";
  printf ("This is %s\n%s", name, license_text);
  exit (0);
}


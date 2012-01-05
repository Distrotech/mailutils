/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/mailutils.h>
#include <mailutils/tls.h>
#include "mailutils/libargp.h"
#include "mu.h"

static char args_doc[] = N_("COMMAND [CMDOPTS]");
static char doc[] = N_("mu -- GNU Mailutils multi-purpose tool.");

static struct argp_option options[] = {
  { NULL }
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static char *
mu_help_filter (int key, const char *text, void *input)
{
  char *s;
  
  switch (key)
    {
    case ARGP_KEY_HELP_PRE_DOC:
      s = dispatch_docstring (text);
      break;
      
    default:
      s = (char*) text;
    }
  return s;
}


static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  mu_help_filter,
  NULL
};

static const char *mu_tool_capa[] = {
  "common",
  "debug",
  "locking",
  "mailbox",
  "auth",
  NULL 
};

struct mu_cfg_param mu_tool_param[] = {
  { NULL }
};

int
mu_help ()
{
  char *x_argv[3];
    
  x_argv[0] = (char*) mu_program_name;
  x_argv[1] = "--help";
  x_argv[2] = NULL;
  return mu_app_init (&argp, mu_tool_capa, mu_tool_param, 
		      2, x_argv, 0, NULL, NULL);
}

int
main (int argc, char **argv)
{
  int index;
  mutool_action_t action;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();
  MU_AUTH_REGISTER_ALL_MODULES ();

  /* Register the desired mailbox formats.  */
  mu_register_all_mbox_formats ();

#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif
  mu_argp_init (NULL, NULL);
  if (mu_app_init (&argp, mu_tool_capa, mu_tool_param, 
		   argc, argv, ARGP_IN_ORDER, &index, NULL))
    exit (1);

  argc -= index;
  argv += index;

  if (argc < 1)
    {
      mu_error (_("what do you want me to do?"));
      exit (1);
    }
	
  action = dispatch_find_action (argv[0]);
  if (!action)
    {
      mu_error (_("don't know what %s is"), argv[0]);
      exit (1);
    }

  /* Disable --version option in action. */
  argp_program_version = NULL;
  argp_program_version_hook = NULL;

  /* Reset argv[0] for diagnostic purposes */
  mu_asprintf (&argv[0], "%s %s", mu_program_name, argv[0]);
  
  /* Run the action. */
  exit (action (argc, argv));
}

  
  

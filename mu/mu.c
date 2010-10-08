/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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
static char doc[] = N_("mu -- GNU Mailutils multi-purpose tool.\n\
Commands are:\n\
    mu info   - show Mailutils configuration\n\
    mu query  - query configuration values\n\
    mu pop    - POP3 client program\n\
    mu filter - filter program\n\
    mu 2047   - decode/encode message headers as per RFC 2047\n\
    mu acl    - test access control lists\n\
\n\
Try `mu COMMAND --help' to get help on a particular COMMAND.\n\
\n\
Options are:\n");
/* FIXME: add
    mu imap
    mu send [opts]
 */

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

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  NULL,
  NULL
};

static const char *mu_tool_capa[] = {
  "common",
  "debug",
  "license",
  "locking",
  "mailbox",
  "auth",
  NULL 
};

struct mu_cfg_param mu_tool_param[] = {
  { NULL }
};

struct mutool_action_tab
{
  const char *name;
  mutool_action_t action;
};

static int
mutool_nosys (int argc, char **argv)
{
  mu_error (_("%s is not available because required code is not compiled"),
	    argv[0]);
  return 1;
}

struct mutool_action_tab mutool_action_tab[] = {
  { "info", mutool_info },
#ifdef ENABLE_POP
  { "pop", mutool_pop },
#else
  { "pop", mutool_nosys },
#endif
  { "filter", mutool_filter },
  { "2047", mutool_flt2047 },
  { "query", mutool_query },
  { "acl", mutool_acl },
  { NULL }
};

static mutool_action_t
find_action (const char *name)
{
  struct mutool_action_tab *p;

  for (p = mutool_action_tab; p->name; p++)
    if (strcmp (p->name, name) == 0)
      return p->action;
  return NULL;
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
	
  action = find_action (argv[0]);
  if (!action)
    {
      mu_error (_("don't know what %s is"), argv[0]);
      exit (1);
    }

  /* Disable --version option in action. */
  argp_program_version = NULL;
  argp_program_version_hook = NULL;
  /* Run the action. */
  exit (action (argc, argv));
}

  
  

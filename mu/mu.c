/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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
#include "mailutils/cli.h"
#include "mu.h"

struct mu_cli_setup cli = {
  .prog_doc = N_("GNU Mailutils multi-purpose tool."),
  .prog_args = N_("COMMAND [CMDOPTS]"),
  .inorder = 1,
  .prog_doc_hook = subcommand_help
};

static char *capa[] = {
  "debug",
  "locking",
  "mailbox",
  "auth",
  NULL 
};

int
main (int argc, char **argv)
{
  mutool_action_t action;
  static struct mu_parseopt pohint = {
    .po_flags = MU_PARSEOPT_PACKAGE_NAME
                | MU_PARSEOPT_PACKAGE_URL
                | MU_PARSEOPT_BUG_ADDRESS
                | MU_PARSEOPT_EXTRA_INFO
                | MU_PARSEOPT_VERSION_HOOK,
    .po_package_name = PACKAGE_NAME,
    .po_package_url = PACKAGE_URL,
    .po_bug_address = PACKAGE_BUGREPORT,
    .po_extra_info = mu_general_help_text,
    .po_version_hook = mu_version_hook,
  };
  struct mu_cfg_parse_hints cfhint = { .flags = 0 };

  /* Native Language Support */
  MU_APP_INIT_NLS ();
  MU_AUTH_REGISTER_ALL_MODULES ();

  /* Register the desired mailbox formats.  */
  mu_register_all_mbox_formats ();

  mu_cli_ext (argc, argv, &cli, &pohint, &cfhint, capa, NULL, &argc, &argv);

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

  /* Run the action. */
  exit (action (argc, argv));
}

int
mu_help (void)
{
  char *argv[3];
  argv[0] = mu_full_program_name;
  argv[1] = "--help";
  argv[2] = NULL;
  mu_cli (2, argv, &cli, capa, NULL, NULL, NULL);
  return 0;
} 

void
mu_action_getopt (int *pargc, char ***pargv, struct mu_option *opt,
		  char const *docstring, char const *argdoc)
{
  struct mu_parseopt po;
  int flags = MU_PARSEOPT_IMMEDIATE;
  struct mu_option *options[2];
  char const *prog_args[2];
  char *progname;
  
  options[0] = opt;
  options[1] = NULL;

  mu_asprintf (&progname, "%s %s", mu_program_name, (*pargv)[0]);
  po.po_prog_name = progname;
  flags |= MU_PARSEOPT_PROG_NAME;
  
  po.po_prog_doc = docstring;
  flags |= MU_PARSEOPT_PROG_DOC;

  if (argdoc)
    {
      prog_args[0] = argdoc;
      prog_args[1] = NULL;
      po.po_prog_args = prog_args;
      flags |= MU_PARSEOPT_PROG_ARGS;
    }
  
  po.po_bug_address = PACKAGE_BUGREPORT;
  flags |= MU_PARSEOPT_BUG_ADDRESS;

  if (mu_parseopt (&po, *pargc, *pargv, options, flags))
    exit (po.po_exit_error);

  *pargc -= po.po_arg_start;
  *pargv += po.po_arg_start;
  
  mu_parseopt_free (&po);  
}

  
  

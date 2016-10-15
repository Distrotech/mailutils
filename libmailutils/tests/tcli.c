/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2016 Free Software Foundation, Inc.

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
#include <mailutils/mailutils.h>
#include <mailutils/cli.h>

int dtrt_option;

struct mu_option group_a[] = {
  { "dtrt", 'd', "VALUE", MU_OPTION_DEFAULT,
    "do the right thing",
    mu_c_int, &dtrt_option },
  MU_OPTION_END
};

struct mu_option *options[] = { group_a, NULL };

static struct mu_cfg_param config[] = {
  { "do-the-right-thing", mu_c_int, &dtrt_option, 0, NULL,
    "do the right thing" },
  { NULL }
};

char const *alt_args[] = { "ALT ARGUMENTS 1", "ALT ARGUMENTS 2", NULL };

struct mu_cli_setup cli = {
  options,
  config,
  "Tests standard command line interface",
  "ARGUMENTS",
  alt_args
};

static char **
getcapa (void)
{
  struct mu_wordsplit ws;
  char *p;
  
  p = getenv ("MU_CLI_CAPA");
  if (!p)
    return NULL;
  ws.ws_delim = ",";
  if (mu_wordsplit (p, &ws, 
		    MU_WRDSF_DELIM | MU_WRDSF_NOVAR | MU_WRDSF_NOCMD
		    | MU_WRDSF_WS | MU_WRDSF_SHOWERR))
    exit (1);
  return ws.ws_wordv;
}
  
int
main (int argc, char **argv)
{
  int i;
  
  mu_cli (argc, argv, &cli, getcapa (), NULL, &argc, &argv);
  printf ("DTRT=%d\n", dtrt_option);
  printf ("%d arguments:\n", argc);
  for (i = 0; i < argc; i++)
    printf ("%d: %s\n", i, argv[i]);
  return 0;
}

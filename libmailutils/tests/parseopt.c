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
#include <mailutils/opt.h>

char *file_name;
char *opt_value = "initial";
char *find_value;
int jobs = 0;
int x_option;
int a_option;
int d_option;

struct mu_option group_a[] = {
  MU_OPTION_GROUP("Group A"),
  { "file", 'f', "FILE", MU_OPTION_DEFAULT,
    "set file name",
    mu_c_string, &file_name
  },
  { "optional", 'o', "FILE", MU_OPTION_ARG_OPTIONAL,
    "optional argument",
    mu_c_string, &opt_value },
  { NULL, 'x', NULL, MU_OPTION_DEFAULT,
    "short-only option",
    mu_c_incr, &x_option },
  { "all", 'a', NULL, MU_OPTION_DEFAULT,
    "no arguments to this one",
    mu_c_bool, &a_option },
  MU_OPTION_END
};

struct mu_option group_b[] = {
  MU_OPTION_GROUP("Group B"),
  { "debug", 'd', NULL, MU_OPTION_DEFAULT,
    "another option",
    mu_c_incr, &d_option },
  { "verbose", 'v', NULL, MU_OPTION_ALIAS },
  { "find", 'F', "VALUE", MU_OPTION_DEFAULT,
    "find VALUE",
    mu_c_string, &find_value },
  { "jobs", 'j', "N", MU_OPTION_DEFAULT,
    "sets numeric value",
    mu_c_int, &jobs },
  MU_OPTION_END
};

struct mu_option *optv[] = { group_a, group_b, NULL };

#define S(s) ((s)?(s):"(null)")

int
main (int argc, char *argv[])
{
  struct mu_parseopt po;
  int rc;
  int i;
  int flags = MU_PARSEOPT_DEFAULT;
  
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  if (getenv ("MU_PARSEOPT_DEFAULT"))
    flags = MU_PARSEOPT_DEFAULT;
  else
    {
      if (getenv ("MU_PARSEOPT_IN_ORDER"))
	flags |= MU_PARSEOPT_IN_ORDER;
      if (getenv ("MU_PARSEOPT_IGNORE_ERRORS"))
	flags |= MU_PARSEOPT_IGNORE_ERRORS;
      if (getenv ("MU_PARSEOPT_IN_ORDER"))
	flags |= MU_PARSEOPT_IN_ORDER;
      if (getenv ("MU_PARSEOPT_NO_ERREXIT"))
	flags |= MU_PARSEOPT_NO_ERREXIT;
      if (getenv ("MU_PARSEOPT_NO_STDOPT"))
	flags |= MU_PARSEOPT_NO_STDOPT;
    }
  
  rc = mu_parseopt (&po, argc, argv, optv, flags);
  printf ("rc=%d\n", rc);
  mu_parseopt_apply (&po);

  argc -= po.po_arg_start;
  argv += po.po_arg_start;

  mu_parseopt_free (&po);

  printf ("file_name=%s\n", S(file_name));
  printf ("opt_value=%s\n", S(opt_value));
  printf ("x_option=%d\n", x_option);
  printf ("a_option=%d\n", a_option);
  printf ("find_value=%s\n", S(find_value));
  printf ("d_option=%d\n", d_option);
  printf ("jobs=%d\n", jobs);
  
  printf ("argv:\n");
  for (i = 0; i < argc; i++)
    {
      printf ("%d: %s\n", i, argv[i]);
    }

  return 0;
}

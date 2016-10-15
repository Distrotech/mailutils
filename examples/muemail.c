/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012, 2014-2016 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <mailutils/util.h>
#include <mailutils/cli.h>

char *capa[] = {
  "address",
  NULL
};

static struct mu_cli_setup cli = { NULL };
  

int
main (int argc, char *argv[])
{
  mu_cli (argc, argv, &cli, capa, NULL, &argc, &argv);

  if (argc == 0)
    printf ("current user -> %s\n", mu_get_user_email (0));
  else
    {
      int i;
      for (i = 0; i < argc; i++)
        printf ("%s -> %s\n", argv[i], mu_get_user_email (argv[i]));
    }

  return 0;
}


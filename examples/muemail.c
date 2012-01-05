/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012 Free Software Foundation,
   Inc.

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
#include "mailutils/libargp.h"

const char *capa[] = {
  "address",
  NULL
};

int
main (int argc, char *argv[])
{
  int arg = 1;

  if (mu_app_init (NULL, capa, NULL, argc, argv, 0, &arg, NULL))
    exit (1);

  if (!argv[arg])
    printf ("current user -> %s\n", mu_get_user_email (0));
  else
    {
      for (; argv[arg]; arg++)
        printf ("%s -> %s\n", argv[arg], mu_get_user_email (argv[arg]));
    }

  return 0;
}


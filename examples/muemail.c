/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mailutils/mutil.h>
#include <mailutils/argp.h>

#include <stdio.h>

const char *capa[] = {
  "address",
  NULL
};

int
main (int argc, char *argv[])
{
  int arg = 1;

  if (mu_argp_parse (NULL, &argc, &argv, 0, capa, &arg, NULL))
    abort ();

  if (!argv[arg])
    printf ("current user -> %s\n", mu_get_user_email (0));
  else
    {
      for (; argv[arg]; arg++)
        printf ("%s -> %s\n", argv[arg], mu_get_user_email (argv[arg]));
    }

  return 0;
}


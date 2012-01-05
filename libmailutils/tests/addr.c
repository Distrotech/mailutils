/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007, 2010-2012 Free Software
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
#include <string.h>
#include <errno.h>

#include <mailutils/address.h>
#include <mailutils/errno.h>
#include <mailutils/kwd.h>
#include <mailutils/util.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>

#define EPARSE MU_ERR_NOENT

int
parse (const char *str)
{
  size_t i;
  size_t pcount = 0;
  int status;
  const char *buf;
  mu_address_t address = NULL;
  struct mu_address hint;

  hint.domain = "localhost";
  status = mu_address_create_hint (&address, str, &hint, MU_ADDR_HINT_DOMAIN);
  mu_address_get_count (address, &pcount);

  if (status)
    {
      fprintf (stderr, "%s\n", mu_errname (status));
      return 1;
    }
  else
    printf ("naddr: %lu\n", (unsigned long) pcount);

  for (i = 1; i <= pcount; i++)
    {
      int isgroup;

      mu_address_is_group (address, i, &isgroup);
      printf ("%lu. ", (unsigned long) i);

      if (isgroup)
        {
          mu_address_sget_personal (address, i, &buf);
          printf ("group <%s>:\n", buf);
        }
      else
        {
          mu_address_sget_email (address, i, &buf);
          printf ("email <%s>:\n", buf);
        }

      if (mu_address_sget_personal (address, i, &buf) == 0 && buf && !isgroup)
        printf ("personal <%s>\n", buf);

      if (mu_address_sget_comments (address, i, &buf) == 0 && buf)
        printf ("comments <%s>\n", buf);

      if (mu_address_sget_local_part (address, i, &buf) == 0 && buf)
        {
          printf ("local-part <%s>", buf);

          if (mu_address_sget_domain (address, i, &buf) == 0 && buf)
            printf (" domain <%s>", buf);

          printf ("\n");
        }

      if (mu_address_sget_route (address, i, &buf) == 0 && buf)
        printf ("route <%s>\n", buf);
    }
  mu_address_destroy (&address);
  return 0;
}

int
main (int argc, char *argv[])
{
  char buf[256];

  if (argc > 2)
    {
      fprintf (stderr, "usage: %s [address]\n", argv[0]);
      return 2;
    }

  if (argc == 2)
    exit (parse (argv[1]));
  
  while (fgets (buf, sizeof buf, stdin))
    {
      mu_rtrim_class (buf, MU_CTYPE_SPACE);
      parse (buf);
    }
  return 0;
}

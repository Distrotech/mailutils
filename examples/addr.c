/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005,
   2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <mailutils/address.h>
#include <mailutils/errno.h>
#include <mailutils/mutil.h>

#define EPARSE MU_ERR_NOENT

static int
parse (const char *str)
{
  size_t no = 0;
  size_t pcount = 0;
  int status;
  const char *buf;
  mu_address_t address = NULL;

  mu_set_user_email_domain ("localhost");
  status = mu_address_create (&address, str);
  mu_address_get_count (address, &pcount);

  if (status)
    {
      printf ("%s=> error %s\n\n", str, mu_errname (status));
      return 0;
    }
  else
    {
      printf ("%s=> pcount %lu\n", str, (unsigned long) pcount);
    }

  for (no = 1; no <= pcount; no++)
    {
      int isgroup;

      mu_address_is_group (address, no, &isgroup);
      printf ("%lu ", (unsigned long) no);

      if (isgroup)
        {
          mu_address_sget_personal (address, no, &buf);
          printf ("group <%s>\n", buf);
        }
      else
        {
          mu_address_sget_email (address, no, &buf);
          printf ("email <%s>\n", buf);
        }

      if (mu_address_sget_personal (address, no, &buf) == 0 && buf && !isgroup)
        printf ("   personal <%s>\n", buf);

      if (mu_address_sget_comments (address, no, &buf) == 0 && buf)
        printf ("   comments <%s>\n", buf);

      if (mu_address_sget_local_part (address, no, &buf) == 0 && buf)
        {
          printf ("   local-part <%s>", buf);

          if (mu_address_sget_domain (address, no, &buf) == 0 && buf)
            printf (" domain <%s>", buf);

          printf ("\n");
        }

      if (mu_address_sget_route (address, no, &buf) == 0 && buf)
        printf ("   route <%s>\n", buf);
    }
  mu_address_destroy (&address);

  printf ("\n");
  return 0;
}

static int
parseinput (void)
{
  char buf[BUFSIZ];

  while (fgets (buf, sizeof (buf), stdin) != 0)
    {
      buf[strlen (buf) - 1] = 0;
      parse (buf);
    }

  return 0;
}

int
main (int argc, const char *argv[])
{
  argc = 1;

  if (!argv[argc])
    return parseinput ();

  for (; argv[argc]; argc++)
    {
      if (strcmp (argv[argc], "-") == 0)
	parseinput ();
      else
	parse (argv[argc]);
    }

  return 0;
}

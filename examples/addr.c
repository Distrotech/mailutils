/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

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
  char buf[BUFSIZ];
  address_t address = NULL;

  mu_set_user_email_domain ("localhost");
  status = address_create (&address, str);
  address_get_count (address, &pcount);

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
      size_t got = 0;
      int isgroup;

      address_is_group (address, no, &isgroup);
      printf ("%lu ", (unsigned long) no);

      if (isgroup)
        {
          address_get_personal (address, no, buf, sizeof (buf), &got);
          printf ("group <%s>\n", buf);
        }
      else
        {
          address_get_email (address, no, buf, sizeof (buf), 0);
          printf ("email <%s>\n", buf);
        }

      address_get_personal (address, no, buf, sizeof (buf), &got);
      if (got && !isgroup)
        printf ("   personal <%s>\n", buf);

      address_get_comments (address, no, buf, sizeof (buf), &got);
      if (got)
        printf ("   comments <%s>\n", buf);

      address_get_local_part (address, no, buf, sizeof (buf), &got);
      if (got)
        {
          printf ("   local-part <%s>", buf);

          address_get_domain (address, no, buf, sizeof (buf), &got);
          if (got)
            printf (" domain <%s>", buf);

          printf ("\n");
        }

      address_get_route (address, no, buf, sizeof (buf), &got);
      if (got)
        printf ("   route <%s>\n", buf);
    }
  address_destroy (&address);

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

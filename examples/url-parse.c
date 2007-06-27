/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/url.h>

int
main ()
{
  char str[1024];
  char buffer[1024];
  long port = 0;
  int len = sizeof (buffer);
  mu_url_t u = NULL;

  while (fgets (str, sizeof (str), stdin) != NULL)
    {
      int rc;

      str[strlen (str) - 1] = '\0';     /* chop newline */
      if (strspn (str, " \t") == strlen (str))
        continue;               /* skip empty lines */
      if ((rc = mu_url_create (&u, str)) != 0)
        {
          fprintf (stderr, "mu_url_create %s ERROR: [%d] %s",
                   str, rc, mu_strerror (rc));
          exit (1);
        }
      if ((rc = mu_url_parse (u)) != 0)
        {
          printf ("%s => FAILED: [%d] %s\n",
                  str, rc, mu_strerror (rc));
          continue;
        }
      printf ("%s => SUCCESS\n", str);

      mu_url_get_scheme (u, buffer, len, NULL);
      printf ("\tscheme <%s>\n", buffer);

      mu_url_get_user (u, buffer, len, NULL);
      printf ("\tuser <%s>\n", buffer);

      mu_url_get_passwd (u, buffer, len, NULL);
      printf ("\tpasswd <%s>\n", buffer);

      mu_url_get_auth (u, buffer, len, NULL);
      printf ("\tauth <%s>\n", buffer);

      mu_url_get_host (u, buffer, len, NULL);
      printf ("\thost <%s>\n", buffer);

      mu_url_get_port (u, &port);
      printf ("\tport %ld\n", port);

      mu_url_get_path (u, buffer, len, NULL);
      printf ("\tpath <%s>\n", buffer);

      mu_url_get_query (u, buffer, len, NULL);
      printf ("\tquery <%s>\n", buffer);

      mu_url_destroy (&u);

    }
  return 0;
}

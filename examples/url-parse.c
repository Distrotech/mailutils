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
  url_t u = NULL;

  while (fgets (str, sizeof (str), stdin) != NULL)
    {
      int rc;

      str[strlen (str) - 1] = '\0';     /* chop newline */
      if (strspn (str, " \t") == strlen (str))
        continue;               /* skip empty lines */
      if ((rc = url_create (&u, str)) != 0)
        {
          fprintf (stderr, "url_create %s ERROR: [%d] %s",
                   str, rc, mu_errstring (rc));
          exit (1);
        }
      if ((rc = url_parse (u)) != 0)
        {
          printf ("%s => FAILED: [%d] %s\n",
                  str, rc, mu_errstring (rc));
          continue;
        }
      printf ("%s => SUCCESS\n", str);

      url_get_scheme (u, buffer, len, NULL);
      printf ("\tscheme <%s>\n", buffer);

      url_get_user (u, buffer, len, NULL);
      printf ("\tuser <%s>\n", buffer);

      url_get_passwd (u, buffer, len, NULL);
      printf ("\tpasswd <%s>\n", buffer);

      url_get_auth (u, buffer, len, NULL);
      printf ("\tauth <%s>\n", buffer);

      url_get_host (u, buffer, len, NULL);
      printf ("\thost <%s>\n", buffer);

      url_get_port (u, &port);
      printf ("\tport %ld\n", port);

      url_get_path (u, buffer, len, NULL);
      printf ("\tpath <%s>\n", buffer);

      url_get_query (u, buffer, len, NULL);
      printf ("\tquery <%s>\n", buffer);

      url_destroy (&u);

    }
  return 0;
}

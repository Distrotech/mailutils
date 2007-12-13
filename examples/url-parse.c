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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/url.h>

#define CAT2(a,b) a ## b

#define GET_AND_PRINT(field,u,buf,status) 		                \
      status = CAT2(mu_url_sget_,field) (u, &buf);	                \
      if (status == MU_ERR_NOENT)			                \
	buf = "";					                \
      else if (status)					                \
	{						                \
	  mu_error ("cannot get %s: %s", #field, mu_strerror (status));	\
	  exit (1);					                \
        }                                                               \
      printf ("\t" #field " <%s>\n", buf)


int
main ()
{
  char str[1024];
  long port = 0;
  mu_url_t u = NULL;

  while (fgets (str, sizeof (str), stdin) != NULL)
    {
      int rc;
      const char *buf;
      
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

      GET_AND_PRINT (scheme, u, buf, rc);
      GET_AND_PRINT (user, u, buf, rc);
      GET_AND_PRINT (passwd, u, buf, rc);
      GET_AND_PRINT (auth, u, buf, rc);
      GET_AND_PRINT (host, u, buf, rc);

      rc = mu_url_get_port (u, &port);
      if (rc)					
	{						
	  mu_error ("cannot get %s: %s", "port", mu_strerror (rc));	
	  exit (1);					
        }                                               
      printf ("\tport %ld\n", port);
      
      GET_AND_PRINT (path, u, buf, rc);
      GET_AND_PRINT (query, u, buf, rc);

      mu_url_destroy (&u);

    }
  return 0;
}

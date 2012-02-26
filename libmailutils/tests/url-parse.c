/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2009-2012 Free Software Foundation,
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
#include <string.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/url.h>
#include <mailutils/secret.h>
#include <mailutils/kwd.h>

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
      printf (#field " <%s>\n", buf)

static void
print_fvpairs (mu_url_t url)
{
  size_t fvc, i;
  char **fvp;
  int rc = mu_url_sget_fvpairs (url, &fvc, &fvp);
  if (rc)
    {
      mu_error ("cannot get F/V pairs: %s", mu_strerror (rc));
      exit (1);
    }
  if (fvc == 0)
    return;
  for (i = 0; i < fvc; i++)
    printf ("param[%lu] <%s>\n", (unsigned long) i, fvp[i]);
}

static void
print_query (mu_url_t url)
{
  size_t qargc, i;
  char **qargv;
  int rc = mu_url_sget_query (url, &qargc, &qargv);
  if (rc)
    {
      mu_error ("cannot get query: %s", mu_strerror (rc));
      exit (1);
    }
  if (qargc == 0)
    return;
  for (i = 0; i < qargc; i++)
    printf ("query[%lu] <%s>\n", (unsigned long) i, qargv[i]);
}

struct mu_kwd parse_kwtab[] = {
  { "hexcode", MU_URL_PARSE_HEXCODE },
  { "hidepass", MU_URL_PARSE_HIDEPASS },
  { "portsrv", MU_URL_PARSE_PORTSRV },
  { "portwc", MU_URL_PARSE_PORTWC },
  { "pipe",  MU_URL_PARSE_PIPE },
  { "slash", MU_URL_PARSE_SLASH },
  { "dslash_optional", MU_URL_PARSE_DSLASH_OPTIONAL },
  { "default", MU_URL_PARSE_DEFAULT },
  { "local", MU_URL_PARSE_LOCAL },
  { "all", MU_URL_PARSE_ALL },
  { NULL }
};


int
main (int argc, char **argv)
{
  char str[1024];
  unsigned port = 0;
  mu_url_t u = NULL, uhint = NULL;
  int i;
  int parse_flags = 0;
  int rc;
  
  mu_set_program_name (argv[0]);
  for (i = 1; i < argc; i++)
    {
      int flag;

      if (strncmp (argv[i], "hint=", 5) == 0)
	{
	  rc = mu_url_create (&uhint, argv[i] + 5);
	  if (rc)
	    {
	      mu_error ("cannot create hints: %s", mu_strerror (rc));
	      exit (1);
	    }
	}
      else if (mu_kwd_xlat_name_ci (parse_kwtab, argv[i], &flag) == 0)
	parse_flags |= flag;
      else
	{
	  mu_error ("%s: unknown flag %s", argv[0], argv[i]);
	  exit (1);
	}
    }

  while (fgets (str, sizeof (str), stdin) != NULL)
    {
      int rc;
      const char *buf;
      mu_secret_t secret;
      
      str[strlen (str) - 1] = '\0';     /* chop newline */
      if (strspn (str, " \t") == strlen (str))
        continue;               /* skip empty lines */
      if ((rc = mu_url_create_hint (&u, str, parse_flags, uhint)) != 0)
        {
          fprintf (stderr, "mu_url_create %s ERROR: [%d] %s\n",
                   str, rc, mu_strerror (rc));
          exit (1);
        }

      GET_AND_PRINT (scheme, u, buf, rc);
      GET_AND_PRINT (user, u, buf, rc);

      rc = mu_url_get_secret (u, &secret);
      if (rc == MU_ERR_NOENT)
	printf ("passwd <>\n");
      else if (rc)
	{
	  mu_error ("cannot get %s: %s", "passwd", mu_strerror (rc));
	  exit (1);
        }
      else
	{
	  printf ("passwd <%s>\n", mu_secret_password (secret));
	  mu_secret_password_unref (secret);
	}
      
      GET_AND_PRINT (auth, u, buf, rc);
      GET_AND_PRINT (host, u, buf, rc);

      rc = mu_url_get_port (u, &port);
      if (rc)					
	{						
	  mu_error ("cannot get %s: %s", "port", mu_strerror (rc));	
	  exit (1);					
        }                                               
      printf ("port %hu\n", port);
      
      GET_AND_PRINT (path, u, buf, rc);
      print_fvpairs (u);
      print_query (u); 

      mu_url_destroy (&u);

    }
  return 0;
}

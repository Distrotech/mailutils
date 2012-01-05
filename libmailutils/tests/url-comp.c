/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

static const char *
strval (const char *str)
{
  return str[0] ? str : NULL;
}

void
usage (FILE *str, int code)
{
  fprintf (str, "usage: %s [url=URL] [OPTIONS]\n", mu_program_name);
  exit (code);
}

int
main (int argc, char **argv)
{
  int i = 1, rc;
  mu_url_t url = NULL;
  const char *arg;
  
  mu_set_program_name (argv[0]);
  
  if (argc > 1)
    {
      if (strcmp (argv[1], "help") == 0 ||
	  strcmp (argv[1], "--help") == 0 ||
	  strcmp (argv[1], "-h") == 0)
	usage (stdout, 0);

      if (strncmp (argv[1], "url=", 4) == 0)
	{
	  MU_ASSERT (mu_url_create (&url, argv[1] + 4));
	  i = 2;
	}
    }

  if (!url)
    {
      MU_ASSERT (mu_url_create_null (&url));
      i = 1;
    }
  
  for (; i < argc; i++)
    {
      if (strncmp (argv[i], "scheme=", 7) == 0)
	{
	  MU_ASSERT (mu_url_set_scheme (url, strval (argv[i] + 7)));
	}
      else if (strncmp (argv[i], "user=", 5) == 0)
	{
	  MU_ASSERT (mu_url_set_user (url, strval (argv[i] + 5)));
	}
      else if (strncmp (argv[i], "path=", 5) == 0)
	{
	  MU_ASSERT (mu_url_set_path (url, strval (argv[i] + 5)));
	}
      else if (strncmp (argv[i], "host=", 5) == 0)
	{
	  MU_ASSERT (mu_url_set_host (url, strval (argv[i] + 5)));
	}
      else if (strncmp (argv[i], "port=", 5) == 0)
	{
	  MU_ASSERT (mu_url_set_port (url, atoi (argv[i] + 5)));
	}
      else if (strncmp (argv[i], "service=", 8) == 0)
	{
	  MU_ASSERT (mu_url_set_service (url, strval (argv[i] + 8)));
	}
      else if (strncmp (argv[i], "auth=", 5) == 0)
	{
	  MU_ASSERT (mu_url_set_auth (url, strval (argv[i] + 5)));
	}
      else if (strncmp (argv[i], "pass=", 5) == 0)
	{
	  mu_secret_t secret;

	  arg = strval (argv[i] + 5);
	  if (arg)
	    {
	      MU_ASSERT (mu_secret_create (&secret, arg, strlen (arg)));
	    }
	  else
	    secret = NULL;
	  MU_ASSERT (mu_url_set_secret (url, secret));
	}
      else if (strncmp (argv[i], "param=", 6) == 0)
	{
	  arg = strval (argv[i] + 6);
	  if (arg)
	    MU_ASSERT (mu_url_add_param (url, 1, (const char **)&arg));
	  else
	    MU_ASSERT (mu_url_clear_param (url));
	}
      else if (strncmp (argv[i], "query=", 6) == 0)
	{
	  arg = strval (argv[i] + 6);
	  if (arg)
	    MU_ASSERT (mu_url_add_query (url, 1, (const char **)&arg));
	  else
	    MU_ASSERT (mu_url_clear_query (url));
	}
      else
	{
	  mu_error ("unrecognized argument: %s", argv[i]);
	  return 1;
	}
    }
		       
  rc = mu_url_sget_name (url, &arg);
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return 1;
    }
  

  printf ("%s\n", arg);
  return 0;
}

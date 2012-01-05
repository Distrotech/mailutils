/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2010-2012 Free Software
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
#include <mailutils/mailutils.h>

int
main (int argc, char **argv)
{
  struct mu_sockaddr_hints hints;
  struct mu_sockaddr *sa, *ap;
  int rc, i;
  char *node = NULL, *serv = NULL;
  char *urlstr;
  
  mu_set_program_name (argv[0]);

  memset (&hints, 0, sizeof (hints));
  hints.family = AF_UNSPEC;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "passive") == 0)
	hints.flags |= MU_AH_PASSIVE;
      else if (strcmp (argv[i], "detect") == 0)
	hints.flags |= MU_AH_DETECT_FAMILY;
      else if (strncmp (argv[i], "node=", 5) == 0)
	node = argv[i] + 5;
      else if (strncmp (argv[i], "serv=", 5) == 0)
	serv = argv[i] + 5;
      else if (strncmp (argv[i], "url=", 4) == 0)
	urlstr = argv[i] + 4;
      else if (strncmp (argv[i], "proto=", 6) == 0)
	hints.protocol = atoi(argv[i] + 6);
      else if (strncmp (argv[i], "family=", 7) == 0)
	hints.family = atoi (argv[i] + 7);
      else if (strncmp (argv[i], "socktype=", 9) == 0)
	hints.socktype = atoi (argv[i] + 9);
      else
	{
	  mu_error ("unknown argument: %s", argv[i]);
	  exit (1);
	}
    }

  if (urlstr)
    {
      mu_url_t url;
      
      if (node || serv)
	{
	  mu_error ("both url and host/serv are given");
	  exit (1);
	}

      rc = mu_url_create (&url, urlstr);
      if (rc)
	{
	  mu_error ("cannot create url: %s", mu_strerror (rc));
	  exit (2);
	}
      rc = mu_sockaddr_from_url (&sa, url, &hints);
    }
  else
    rc = mu_sockaddr_from_node (&sa, node, serv, &hints);
  if (rc)
    {
      mu_error ("cannot create sockaddr: %s", mu_strerror (rc));
      exit (2);
    }

  for (ap = sa; ap; ap = ap->next)
    printf ("%s\n", mu_sockaddr_str (ap));

  mu_sockaddr_free (sa);
  exit (0);
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <mailutils/mailutils.h>

int
main (int argc, char *argv[])
{
  int c;
  char buf[256];
  char *charset = strdup ("iso-8859-1");
  char *encoding = strdup ("quoted-printable");
  
  while ((c = getopt (argc, argv, "c:e:h")) != EOF)
    switch (c)
      {
      case 'c':
	free (charset);
	charset = strdup (optarg);
	break;
	
      case 'e':
	free (encoding);
	encoding = strdup (optarg);
	break;
	
      case 'h':
	printf ("usage: %s [-c charset] [-e encoding]\n", argv[0]);
	exit (0);
	
      default:
	exit (1);
      }

  while (fgets (buf, sizeof (buf), stdin))
    {
      int len;
      char *p = NULL;
     
      len = strlen (buf);
      if (len > 0 && buf[len - 1] == '\n')
	buf[len - 1] = 0;
      if (buf[0] == '\\')
	{
	  if (buf[1] == 0)
	    {
	      fprintf (stderr, "Unfinished command\n");
	      continue;
	    }
	  
	  for (p = buf + 2; *p && *p == ' '; p++)
	    ;
	  switch (buf[1])
	    {
	    case 'c':
	      free (charset);
	      charset = strdup (p);
	      break;
	      
	    case 'e':
	      free (encoding);
	      encoding = strdup (p);
	      break;

	    default:
	      fprintf (stderr, "Unknown command\n");
	    }
	}
      else
	{
	  int rc = mu_rfc2047_encode (charset, encoding, buf, &p);
	  printf ("%s=> %s\n", buf, mu_strerror (rc));
	  if (p)
	    printf ("%s\n", p);
	  free (p);
	}
    }
    return 0;
}

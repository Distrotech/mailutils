/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <mailutils/mailutils.h>

static void
print_bytes (unsigned char *b, size_t l)
{
  for (; l; l--, b++)
    printf (" %02x", *b);
  printf ("\n");
}

int
main (int argc, char **argv)
{
  int flags = 0;
  
  mu_set_program_name (argv[0]);
  if (argc < 2)
    {
      mu_error ("usage: %s [-sS] CIDR [CIDR...]", argv[0]);
      return 1;
    }

  while (--argc)
    {
      char *arg = *++argv;
      struct mu_cidr cidr;
      int rc;
      char *str;

      if (strcmp (arg, "-s") == 0)
	{
	  flags |= MU_CIDR_FMT_SIMPLIFY;
	  continue;
	}
      else if (strcmp (arg, "-S") == 0)
	{
	  flags &= ~MU_CIDR_FMT_SIMPLIFY;
	  continue;
	}
	       
      rc = mu_cidr_from_string (&cidr, arg);
      if (rc)
	{
	  mu_error ("%s: %s", arg, mu_strerror (rc));
	  continue;
	}

      printf ("%s:\n", arg);
      printf ("family = %d\n", cidr.family);
      printf ("len = %d\n", cidr.len);
      printf ("address =");
      print_bytes (cidr.address, cidr.len);
      printf ("netmask =");
      print_bytes (cidr.netmask, cidr.len);
      rc = mu_cidr_format (&cidr, flags, &str);
      if (rc)
	{
	  mu_error ("cannot covert to string: %s", mu_strerror (rc));
	  return 2;
	}

      printf ("string = %s\n", str);
      free (str);
    }
  return 0;
}
	
	  

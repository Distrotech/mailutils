/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This is just a test program for libsieve. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <assert.h>
#include <sieve.h>

int
debug_printer (void *unused, const char *fmt, va_list ap)
{
  return vfprintf (stderr, fmt, ap);
}

int
main (int argc, char **argv)
{
  int n, rc, debug = 0;
  sieve_machine_t mach;
  
  assert (argc > 1);
  if (strcmp (argv[1], "-d") == 0)
    {
      sieve_yydebug++;
      n = 2;
      debug++;
      assert (argc > 2);
    }
  else
    n = 1;

  rc = sieve_compile (&mach, argv[n], NULL, NULL);
  if (rc == 0)
    {
      if (debug)
	sieve_set_debug (&mach, debug_printer, 100);
      sieve_run (&mach);
    }
  return rc;
}

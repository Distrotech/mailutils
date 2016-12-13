/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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

#include <config.h>
#include <mailutils/mailutils.h>

/* globtest PATTERN [WORD...]
 */   
int
main (int argc, char **argv)
{
  char *pattern = NULL;
  int flags = 0;
  int rc;
  int i;
  
  mu_set_program_name (argv[0]);
  for (i = 1; i < argc; i++)
    {
      char *a = argv[i];
      if (strcmp (a, "-i") == 0)
	flags |= MU_GLOBF_ICASE;
      else if (strcmp (a, "-s") == 0)
	flags |= MU_GLOBF_SUB;
      else if (strcmp (a, "-c") == 0)
	flags |= MU_GLOBF_COLLAPSE;
      else if (strcmp (a, "--") == 0)
	{
	  i++;
	  break;
	}
      else if (*a != '-')
	break;
      else
	{
	  mu_error ("unknown option %s", a);
	  return 1;
	}
    }
  
  if (i == argc)
    {
      mu_printf ("usage: %s [-ics] PATTERN [WORD...]\n", mu_program_name);
      return 1;
    }

  pattern = argv[i++];
  
  if (i == argc)
    {
      char *regstr;
      
      rc = mu_glob_to_regex (&regstr, pattern, flags);
      if (rc)
	{
	  mu_error ("convert: %s", mu_strerror (rc));
	  return 1;
	}
      mu_printf ("%s\n", regstr);
      free (regstr);
    }
  else
    {
      regex_t regex;
      size_t nmatch = 0;
      regmatch_t *matches = NULL;
      
      rc = mu_glob_compile (&regex, pattern, flags);
      if (rc)
	{
	  mu_error ("compile: %s", mu_strerror (rc));
	  return 1;
	}

      if (flags & MU_GLOBF_SUB)
	{
	  nmatch = regex.re_nsub + 1;
	  matches = mu_calloc (nmatch, sizeof matches[0]);
	}
      
      for (; i < argc; i++)
	{
	  char *a = argv[i];
	  rc = regexec (&regex, a, nmatch, matches, 0);
	  mu_printf ("%s: %s\n", a, rc == 0 ? "OK" : "NO");
	  if (flags & MU_GLOBF_SUB)
	    {
	      size_t j;

	      for (j = 0; j < nmatch; j++)
 		printf ("%02zu: %.*s\n", j,
			matches[j].rm_eo - matches[j].rm_so,
			a + matches[j].rm_so);
	    }
	}
    }
  return 0;
}

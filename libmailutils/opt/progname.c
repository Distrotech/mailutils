/* help.c -- general-purpose command line option parser
   Copyright (C) 2016 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/alloc.h>
#include <mailutils/opt.h>

char *mu_program_name;
char *mu_full_program_name;

void
mu_set_program_name (const char *arg)
{
  char *p;

  p = mu_strdup (arg);
  free (mu_full_program_name);
  mu_full_program_name = p;
  
  p = strrchr (mu_full_program_name, '/');
  if (p)
    ++p;
  else
    p = (char*) mu_full_program_name;
  if (strlen (p) > 3 && memcmp (p, "lt-", 3) == 0)
    p += 3;
  free (mu_program_name);
  mu_program_name = mu_strdup (p);
}

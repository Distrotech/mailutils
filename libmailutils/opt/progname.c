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

char *mu_progname;
char *mu_absprogname;

void
mu_set_progname (char const *arg)
{
  char *p;

  free (mu_absprogname);
  mu_absprogname = mu_strdup (arg);
  
  p = strrchr (arg, '/');
  if (p)
    ++p;
  else
    p = (char*) arg;
  if (strlen (p) > 3 && memcmp (p, "lt-", 3) == 0)
    p += 3;
  free (mu_progname);
  mu_progname = mu_strdup (p);
}

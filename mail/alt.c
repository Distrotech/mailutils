/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#include "mail.h"
#include <pwd.h>

static list_t alternate_names = NULL;
static char *my_email;
static char *my_name;

/*
 * alt[ernates] name...
 */

int
mail_alt (int argc, char **argv)
{
  if (argc == 1)
    {
      if (alternate_names)
	{
	  util_slist_print (alternate_names, 0);
	  fprintf (ofile, "\n");
	}
    }
  else
    {
      util_slist_destroy (&alternate_names);
      while (--argc)
	util_slist_add (&alternate_names, *++argv);
    }
  return 0;
}

char *
mail_whoami ()
{
  return my_name;
}

/* FIXME: this lacks domain name part! */
void
mail_set_my_name (char *name)
{
  char hostname[256];

  if (!name)
    {
      struct passwd *pw = getpwuid(getuid());
      if (!pw)
	{
	  util_error("can't determine my username");
	  exit (1);
	}
      name = pw->pw_name;
    }

  my_name = strdup (name);
  gethostname(hostname, sizeof(hostname));
  hostname[sizeof(hostname)-1] = 0;
  my_email = malloc (strlen (name) + strlen (hostname) + 2);
  if (!my_email)
    {
      util_error("not enough memory");
      abort ();
    }
  sprintf (my_email, "%s@%s", name, hostname);
}
   
int
mail_is_my_name (char *name)
{
  if (util_find_env("metoo")->set)
    return 0;
  if (strchr(name, '@') == NULL && strcasecmp (name, my_name) == 0)
    return 1;
  if (strcasecmp (name, my_email) == 0)
    return 1;
  return util_slist_lookup (alternate_names, name);
}

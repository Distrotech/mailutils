/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

static list_t retained_headers = NULL;
static list_t ignored_headers = NULL;

/*
 * ret[ain] [heder-field...]
 */

int
mail_retain (int argc, char **argv)
{
  if (argc == 1)
    {
      if (!retained_headers)
	fprintf (ofile, _("No fields are currently being retained\n"));
      else
	util_slist_print (retained_headers, 1);
      return 0;
    }

  while (--argc)
    util_slist_add (&retained_headers, *++argv);
  return 0;
}

/*
 * di[scard] [header-field...]
 * ig[nore] [header-field...]
 */

int
mail_discard (int argc, char **argv)
{
  if (argc == 1)
    {
      if (!ignored_headers)
	fprintf (ofile, _("No fields are currently being ignored\n"));
      else
	util_slist_print (ignored_headers, 1);
      return 0;
    }

  while (--argc)
    util_slist_add (&ignored_headers, *++argv);
  return 0;
}


int
mail_header_is_visible (char *str)
{
  if (retained_headers)
    return util_slist_lookup (retained_headers, str);
  else
    return !util_slist_lookup (ignored_headers, str);
}

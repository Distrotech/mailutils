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
static list_t unfolded_headers = NULL;

static int
process_list (int argc, char **argv,
	      list_t *list,
	      void (*fun) (list_t *, char *),
	      char *msg)
{
  if (argc == 1)
    {
      if (list_is_empty (*list))
	fprintf (ofile, _(msg));
      else
	util_slist_print (*list, 1);
      return 0;
    }

  while (--argc)
    fun (list, *++argv);
  return 0;
}

/*
 * ret[ain] [heder-field...]
 */

int
mail_retain (int argc, char **argv)
{
  return process_list (argc, argv, &retained_headers,
		       util_slist_add,
		       N_("No fields are currently being retained\n"));
}

/*
 * di[scard] [header-field...]
 * ig[nore] [header-field...]
 */

int
mail_discard (int argc, char **argv)
{
  return process_list (argc, argv, &ignored_headers,
		       util_slist_add,
		       N_("No fields are currently being ignored\n"));
  return 0;
}

/*
 * unfold [header-field...]
 */

int
mail_unfold (int argc, char **argv)
{
  return process_list (argc, argv, &unfolded_headers,
		       util_slist_add,
		       N_("No fields are currently being unfolded\n"));
}

/*
 * nounfold [header-field...]
 */

int
mail_nounfold (int argc, char **argv)
{
  return process_list (argc, argv, &unfolded_headers,
		       util_slist_remove,
		       N_("No fields are currently being unfolded\n"));
}

int
mail_header_is_unfoldable (char *str)
{
  return util_slist_lookup (unfolded_headers, str);
}

int
mail_header_is_visible (char *str)
{
  if (retained_headers)
    return util_slist_lookup (retained_headers, str);
  else
    return !util_slist_lookup (ignored_headers, str);
}

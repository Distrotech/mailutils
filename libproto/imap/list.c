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

#include <mailutils/imap.h>

int
mu_imap_list (mu_imap_t imap, const char *refname, const char *mboxname,
	      mu_list_t retlist)
{
  return mu_imap_genlist (imap, 0, refname, mboxname, retlist);
}

int
mu_imap_list_new (mu_imap_t imap, const char *refname, const char *mboxname,
		  mu_list_t *plist)
{
  return mu_imap_genlist_new (imap, 0, refname, mboxname, plist);
}



/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007-2008, 2010-2012 Free
   Software Foundation, Inc.

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
#include <mailutils/sys/list.h>
#include <mailutils/errno.h>

int
mu_list_foreach (mu_list_t list, mu_list_action_t action, void *cbdata)
{
  mu_iterator_t itr;
  int status = 0;

  if (list == NULL || action == NULL)
    return EINVAL;
  status = mu_list_get_iterator (list, &itr);
  if (status)
    return status;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      void *item;
      mu_iterator_current (itr, &item);
      if ((status = action (item, cbdata)))
	break;
    }
  mu_iterator_destroy (&itr);
  return status;
}

/* Retained for compatibility with previous versions.
   In the future it will be removed, or changed to a define or weak alias. */
int
mu_list_do (mu_list_t list, mu_list_action_t action, void *cbdata)
{
  return mu_list_foreach (list, action, cbdata);
}

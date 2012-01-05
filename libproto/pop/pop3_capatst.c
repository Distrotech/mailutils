/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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
#include <mailutils/list.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_capa_test (mu_pop3_t pop3, const char *name, const char **pret)
{
  int rc;

  rc = mu_pop3_capa (pop3, 0, NULL);
  if (rc)
    return rc;
  return mu_list_locate (pop3->capa, (void*) name, (void**)pret);
}

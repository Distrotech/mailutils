
/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/cctype.h>
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_set_domain (mu_smtp_t smtp, const char *newdom)
{
  char *dom;
  
  if (!smtp)
    return EINVAL;

  dom = strdup (newdom);
  if (!dom)
    return ENOMEM;
  free (smtp->domain);
  smtp->domain = dom;
  return 0;
}

int
mu_smtp_get_domain (mu_smtp_t smtp, const char **pdom)
{
  if (!smtp)
    return EINVAL;
  *pdom = smtp->domain;
  return 0;
}


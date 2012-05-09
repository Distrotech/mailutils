/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#include <mailutils/errno.h>
#include <mailutils/url.h>
#include <mailutils/secret.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_set_url (mu_smtp_t smtp, mu_url_t url)
{
  if (!smtp)
    return EINVAL;
  mu_url_destroy (&smtp->url);
  if (!url)
    return 0;
  return mu_url_dup (url, &smtp->url);
}

int
mu_smtp_get_url (mu_smtp_t smtp, mu_url_t *purl)
{
  if (!smtp || !purl)
    return EINVAL;
  if (!smtp->url)
    return MU_ERR_NOENT;
  *purl = smtp->url;
  return 0;
}

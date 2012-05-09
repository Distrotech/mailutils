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
#include <mailutils/secret.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_set_secret (mu_smtp_t smtp, mu_secret_t secret)
{
  if (!smtp)
    return EINVAL;
  if (smtp->secret)
    {
      if (MU_SMTP_FISSET (smtp, _MU_SMTP_CLNPASS))
	mu_secret_password_unref (smtp->secret);
      mu_secret_destroy (&smtp->secret);
    }
  if (!secret)
    return 0;
  return mu_secret_dup (secret, &smtp->secret);
}

int
mu_smtp_get_secret (mu_smtp_t smtp, mu_secret_t *secret)
{
  if (!smtp)
    return EINVAL;
  if (!smtp->secret)
    return MU_ERR_NOENT;
  *secret = smtp->secret;
  mu_secret_ref (smtp->secret);
  return 0;
}

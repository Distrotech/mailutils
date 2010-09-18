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
#include <mailutils/errno.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_auth (mu_smtp_t smtp)
{
  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_AUTH))
    return MU_ERR_SEQ;
  if (smtp->state != MU_SMTP_MAIL)
    return MU_ERR_SEQ;
  
#if defined(WITH_GSASL)
  return _mu_smtp_gsasl_auth (smtp);
#else
  /* FIXME: Provide support for some basic authentication methods */
  return ENOSYS;
#endif
}

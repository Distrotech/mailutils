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
#include <mailutils/secret.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_set_param (mu_smtp_t smtp, int pcode, const char *newparam)
{
  char *param;
  
  if (!smtp)
    return EINVAL;
  if (pcode < 0 || pcode >= MU_SMTP_MAX_PARAM)
    return EINVAL;

  if (pcode == MU_SMTP_PARAM_PASSWORD)
    {
      /* Special handling for passwords */
      smtp->param[MU_SMTP_PARAM_PASSWORD] = NULL;
      if (smtp->secret)
	{
	  if (MU_SMTP_FISSET (smtp, _MU_SMTP_CLNPASS))
	    mu_secret_password_unref (smtp->secret);
	  mu_secret_destroy (&smtp->secret);
	}
      MU_SMTP_FCLR (smtp, _MU_SMTP_CLNPASS);
      return mu_secret_create (&smtp->secret, newparam, strlen (newparam));
    }
  
  param = strdup (newparam);
  if (!param)
    return ENOMEM;
  free (smtp->param[pcode]);
  smtp->param[pcode] = param;
  return 0;
}

int
mu_smtp_get_param (mu_smtp_t smtp, int pcode, const char **pparam)
{
  if (!smtp)
    return EINVAL;
  if (pcode < 0 || pcode >= MU_SMTP_MAX_PARAM)
    return EINVAL;
  if (pcode == MU_SMTP_PARAM_PASSWORD && smtp->secret &&
      !MU_SMTP_FISSET (smtp, _MU_SMTP_CLNPASS))
    {
      smtp->param[pcode] = (char*) mu_secret_password (smtp->secret);
      MU_SMTP_FSET (smtp, _MU_SMTP_CLNPASS);
    }
  
  *pparam = smtp->param[pcode];
  return 0;
}

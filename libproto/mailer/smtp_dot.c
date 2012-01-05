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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/cctype.h>
#include <mailutils/list.h>
#include <mailutils/util.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_dot (mu_smtp_t smtp)
{
  int status;
  
  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  if (smtp->state != MU_SMTP_DOT)
    return MU_ERR_SEQ;
  status = mu_smtp_response (smtp);
  MU_SMTP_CHECK_ERROR (smtp, status);
  if (smtp->replcode[0] != '2')
    return MU_ERR_REPLY;
  smtp->state = MU_SMTP_MAIL; /* FIXME: Force _EHLO perhaps? */
  return 0;
}

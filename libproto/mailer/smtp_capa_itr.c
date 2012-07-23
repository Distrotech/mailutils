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
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_capa_iterator (mu_smtp_t smtp, mu_iterator_t *itr)
{
  if (!smtp || !itr)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  if (!smtp->capa)
    {
      int rc = mu_smtp_ehlo (smtp);
      if (rc)
	return rc;
    }
  if (!MU_SMTP_FISSET (smtp, _MU_SMTP_ESMTP))
    return MU_ERR_FAILURE;
  return mu_list_get_iterator (smtp->capa, itr);
}

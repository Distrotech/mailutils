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

static int
capa_comp (const void *item, const void *value)
{
  const char *capa = item;
  const char *needle = value;
  for (; *needle; capa++, needle++)
    {
      if (!*capa)
	return 1;
      if (mu_tolower (*capa) != mu_tolower (*needle))
	return 1;
    }
  return !(*capa == 0 || mu_isspace (*capa));
}

int
mu_smtp_ehlo (mu_smtp_t smtp)
{
  int status;
  
  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;

  if (!smtp->param[MU_SMTP_PARAM_DOMAIN])
    {
      char *host;
      char *p;
      
      status = mu_get_host_name (&host);
      MU_SMTP_CHECK_ERROR (smtp, status);
      p = strchr (host, '.');
      if (p)
	p++;
      else
	p = host;
      status = mu_smtp_set_param (smtp, MU_SMTP_PARAM_DOMAIN, p);
      MU_SMTP_CHECK_ERROR (smtp, status);
    }
  
  status = mu_smtp_write (smtp, "EHLO %s\r\n",
			  smtp->param[MU_SMTP_PARAM_DOMAIN]);
  MU_SMTP_CHECK_ERROR (smtp, status);
  status = mu_smtp_response (smtp);
  MU_SMTP_CHECK_ERROR (smtp, status);
  if (smtp->replcode[0] == '2')
    {
      smtp->flags |= _MU_SMTP_ESMTP;
      smtp->capa = smtp->mlrepl;
      smtp->mlrepl = NULL;
      mu_list_set_comparator (smtp->capa, capa_comp);
    }
  else if (smtp->replcode[0] == '4')
    return MU_ERR_REPLY;
  else
    {
      status = mu_smtp_write (smtp, "HELO %s\r\n",
			      smtp->param[MU_SMTP_PARAM_DOMAIN]);
      MU_SMTP_CHECK_ERROR (smtp, status);
      status = mu_smtp_response (smtp);
      MU_SMTP_CHECK_ERROR (smtp, status);
      smtp->flags &= ~_MU_SMTP_ESMTP;
      if (smtp->replcode[0] != '2')
	return MU_ERR_REPLY;
    }
  if (smtp->state == MU_SMTP_EHLO)
    smtp->state = MU_SMTP_MAIL;
  return 0;
}

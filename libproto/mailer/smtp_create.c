/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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
#include <mailutils/list.h>
#include <mailutils/secret.h>
#include <mailutils/smtp.h>
#include <mailutils/url.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_create (mu_smtp_t *psmtp)
{
  struct _mu_smtp *smtp;

  if (!psmtp)
    return EINVAL;
  
  smtp = calloc (1, sizeof (*smtp));

  if (!smtp)
    return ENOMEM;
  
  smtp->state = MU_SMTP_INIT;
  *psmtp = smtp;
  return 0;
}

void
mu_smtp_destroy (mu_smtp_t *psmtp)
{
  int i;
  struct _mu_smtp *smtp;

  if (!psmtp || !*psmtp)
    return;
  smtp = *psmtp;
  mu_stream_destroy (&smtp->carrier);
  mu_list_destroy (&smtp->capa);
  mu_list_destroy (&smtp->authimpl);
  free (smtp->rdbuf);
  free (smtp->flbuf);
  mu_list_destroy (&smtp->mlrepl);

  mu_list_destroy (&smtp->authmech);
  if (smtp->secret)
    {
      if (MU_SMTP_FISSET (smtp, _MU_SMTP_CLNPASS))
	mu_secret_password_unref (smtp->secret);
      mu_secret_destroy (&smtp->secret);
    }
  mu_url_destroy (&smtp->url);
  
  for (i = 0; i < MU_SMTP_MAX_PARAM; i++)
    {
      if (i == MU_SMTP_PARAM_PASSWORD)
	continue;
      free (smtp->param[i]);
    }
  
  free (smtp);
  *psmtp = NULL;
}


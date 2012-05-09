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
#include <mailutils/diag.h>
#include <mailutils/errno.h>
#include <mailutils/smtp.h>
#include <mailutils/sys/smtp.h>
#include <mailutils/mailbox.h> /* Strange, mu_ticket_file is declared here */
#include <mailutils/util.h>
#include <mailutils/auth.h>
#include <mailutils/url.h>

static int
get_ticket (mu_ticket_t *pticket)
{
  char *filename = mu_tilde_expansion (mu_ticket_file,
				       MU_HIERARCHY_DELIMITER, NULL);
  mu_wicket_t wicket;
  int rc;

  rc = mu_file_wicket_create (&wicket, filename);
  
  if (rc == 0)
    rc = mu_wicket_get_ticket (wicket, NULL, pticket);
  mu_wicket_destroy (&wicket);
  free (filename);
  return rc;
}

#define _HAS_USERNAME 0x01
#define _HAS_PASSWORD 0x02

static int
_mu_smtp_fixup_params (mu_smtp_t smtp)
{
  const char *str;
  mu_ticket_t ticket = NULL;
  int flags = 0;
  int rc;

  if (smtp->param[MU_SMTP_PARAM_USERNAME])
    flags |= _HAS_USERNAME;
  
  if (smtp->secret)
    flags |= _HAS_PASSWORD;

  if ((flags & (_HAS_USERNAME|_HAS_PASSWORD)) == (_HAS_USERNAME|_HAS_PASSWORD))
    return 0; /* Nothing to do */
  
  if (!smtp->url)
    return 0;
  
  if (!(flags & _HAS_USERNAME))
    {
      rc = mu_url_sget_user (smtp->url, &str);
      if (rc == 0 &&
	  mu_smtp_set_param (smtp, MU_SMTP_PARAM_USERNAME, str) == 0)
	flags |= _HAS_USERNAME;
    }

  if (!(flags & _HAS_PASSWORD) &&
      mu_url_get_secret (smtp->url, &smtp->secret) == 0)
    flags |= _HAS_PASSWORD;

  if ((!(flags & _HAS_USERNAME) ||
       !(flags & _HAS_PASSWORD)) &&
      get_ticket (&ticket) == 0)
    {
      if (!(flags & _HAS_USERNAME) &&
	  mu_ticket_get_cred (ticket, smtp->url, "SMTP User: ",
			      &smtp->param[MU_SMTP_PARAM_USERNAME],
			      NULL) == 0)
	flags |= _HAS_USERNAME;

      if (!(flags & _HAS_PASSWORD) && !smtp->secret)
	mu_ticket_get_cred (ticket, smtp->url, "SMTP Passwd: ",
			    NULL, &smtp->secret);
      mu_ticket_destroy (&ticket);
    }

  return 0;
}

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

  /* Obtain missing authentication credentials either from the
     URL (when supplied) or from the user ticket file, or by
     asking the user, if anything else fails.
     
     FIXME: This needs some more work.  First of all, it should
     be called only when really needed (e.g. by mu_smtp_get_param).
     Secondly, it should ask the user even if no URL was supplied
     (presently it does not).  Thirdly, there should be an API to
     let caller determine the way of inputting missing data (by
     default it does that on tty, which obviously will not suite
     GUI applications). */
  _mu_smtp_fixup_params (smtp);
  if (!smtp->param[MU_SMTP_PARAM_USERNAME] && !smtp->secret)
    return MU_ERR_AUTH_NO_CRED;
#if defined(WITH_GSASL)
  return _mu_smtp_gsasl_auth (smtp);
#else
  /* FIXME: Provide support for some basic authentication methods */
  return ENOSYS;
#endif
}

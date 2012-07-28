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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/diag.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_write (mu_smtp_t smtp, const char *fmt, ...)
{
  va_list ap;
  int rc;
  
  va_start (ap, fmt);
  rc = mu_stream_vprintf (smtp->carrier, fmt, ap);
  va_end (ap);
  return rc;
}

static int
_mu_smtp_init_mlist (mu_smtp_t smtp)
{
  if (!smtp->mlrepl)
    {
      int rc = mu_list_create (&smtp->mlrepl);
      if (rc == 0)
	mu_list_set_destroy_item (smtp->mlrepl, mu_list_free_item);
      return rc;
    }
  else
    mu_list_clear (smtp->mlrepl);
  return 0;
}

int
mu_smtp_response (mu_smtp_t smtp)
{
  int rc;
  size_t n;
  
  rc = mu_stream_getline (smtp->carrier, &smtp->rdbuf, &smtp->rdsize, &n);
  MU_SMTP_CHECK_ERROR (smtp, rc);
  if (n == 0)
    MU_SMTP_CHECK_ERROR (smtp, EIO);
  n = mu_rtrim_class (smtp->rdbuf, MU_CTYPE_ENDLN);
  if (n < 3 || !mu_isdigit (smtp->rdbuf[0]))
    {
      mu_diag_output (MU_DIAG_NOTICE,
		      "received invalid reply from SMTP server");
      MU_SMTP_CHECK_ERROR (smtp, MU_ERR_BADREPLY);
    }
  memcpy (smtp->replcode, smtp->rdbuf, 3);
  smtp->replcode[3] = 0;
  if (smtp->rdbuf[3] == '-')
    {
      smtp->flags |= _MU_SMTP_MLREPL;
      n -= 3;
      if (smtp->flsize < n)
	{
	  char *p = realloc (smtp->flbuf, n);
	  if (!p)
	    MU_SMTP_CHECK_ERROR (smtp, ENOMEM);
	  smtp->flbuf = p;
	  smtp->flsize = n;
	}
      memcpy (smtp->flbuf, smtp->rdbuf + 4, n - 1);
      smtp->flbuf[n - 1] = 0;
      smtp->replptr = smtp->flbuf;

      rc = _mu_smtp_init_mlist (smtp);
      MU_SMTP_CHECK_ERROR (smtp, rc);
      do
	{
          char *p;

	  rc = mu_stream_getline (smtp->carrier, &smtp->rdbuf, &smtp->rdsize,
				  &n);
	  MU_SMTP_CHECK_ERROR (smtp, rc);
	  if (n == 0)
	    MU_SMTP_CHECK_ERROR (smtp, EIO);
	  n = mu_rtrim_class (smtp->rdbuf, MU_CTYPE_ENDLN);
	  if (n < 3 || memcmp (smtp->rdbuf, smtp->replcode, 3))
	    {
	      mu_diag_output (MU_DIAG_NOTICE,
			      "received invalid reply from SMTP server");
	      MU_SMTP_CHECK_ERROR (smtp, MU_ERR_BADREPLY);
	    }
	  p = strdup (smtp->rdbuf + 4);
	  if (!p)
            MU_SMTP_CHECK_ERROR (smtp, ENOMEM);
	  mu_list_append (smtp->mlrepl, p);
	}
      while (smtp->rdbuf[3] == '-');
    }
  else
    {
      smtp->flags &= ~_MU_SMTP_MLREPL;
      smtp->replptr = smtp->rdbuf + 4;
    }
  return 0;
}
    
int
mu_smtp_replcode (mu_smtp_t smtp, char *buf)
{
  if (!smtp || !buf)
    return EINVAL;
  strcpy (buf, smtp->replcode);
  return 0;
}

int
mu_smtp_sget_reply (mu_smtp_t smtp, const char **pbuf)
{
  if (!smtp || !pbuf)
    return EINVAL;
  *pbuf = smtp->replptr;
  return 0;
}
  
int
mu_smtp_get_reply_iterator (mu_smtp_t smtp, mu_iterator_t *pitr)
{
  if (!smtp || !pitr)
    return EINVAL;
  return mu_list_get_iterator (smtp->mlrepl, pitr);
}

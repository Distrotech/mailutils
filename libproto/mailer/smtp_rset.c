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
#include <mailutils/filter.h>
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_rset (mu_smtp_t smtp)
{
  int status;
  
  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  status = mu_smtp_write (smtp, "RSET\r\n");
  MU_SMTP_CHECK_ERROR (smtp, status);
  status = mu_smtp_response (smtp);
  MU_SMTP_CHECK_ERROR (smtp, status);
  if (smtp->replcode[0] != '2')
    return MU_ERR_REPLY;
  
  switch (smtp->state)
    {
    case MU_SMTP_INIT:
    case MU_SMTP_EHLO:
    case MU_SMTP_DOT:
      /* RFC 2821, 4.1.1.5 RESET (RSET):
	[RSET] is effectively equivalent to a NOOP (i.e., if has no effect)
	if issued immediately after EHLO, before EHLO is issued in the
	session, after an end-of-data indicator has been sent and
	acknowledged, or immediately before a QUIT */
      break;
      
    default:
      smtp->state = MU_SMTP_MAIL;
    }
  return 0;
}

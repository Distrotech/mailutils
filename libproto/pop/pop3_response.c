/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010-2012, 2014 Free Software Foundation,
   Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/pop3.h>
#include <mailutils/util.h>

/* If we did not grap the ack already, call pop3_readline() but handle
   Nonblocking also.  */
int
mu_pop3_response (mu_pop3_t pop3, size_t *pnread)
{
  size_t n = 0;
  int status = 0;

  if (pop3 == NULL)
    return EINVAL;

  if (!MU_POP3_FISSET (pop3, MU_POP3_ACK))
    {
      status = mu_stream_getline (pop3->carrier, &pop3->ackbuf,
				  &pop3->acksize, NULL);
      if (status == 0)
	{
	  n = mu_rtrim_class (pop3->ackbuf, MU_CTYPE_SPACE);
	  MU_POP3_FSET (pop3, MU_POP3_ACK); /* Flag that we have the ack.  */
	}
    }
  else if (pop3->ackbuf)
    n = strlen (pop3->ackbuf);

  if (n < 3)
    status = MU_ERR_BADREPLY;
  else if (strncmp (pop3->ackbuf, "-ERR", 4) == 0)
    status = MU_ERR_REPLY;
  else if (strncmp (pop3->ackbuf, "+OK", 3))
    status = MU_ERR_BADREPLY;
  
  if (pnread)
    *pnread = n;
  return status;
}

const char *
mu_pop3_strresp (mu_pop3_t pop3)
{
    if (pop3 == NULL)
      return NULL;
    if (!MU_POP3_FISSET (pop3, MU_POP3_ACK))
      return NULL;
    return pop3->ackbuf;
}

int
mu_pop3_sget_response (mu_pop3_t pop3, const char **sptr)
{
  if (pop3 == NULL)
    return EINVAL;
  if (!MU_POP3_FISSET (pop3, MU_POP3_ACK))
    return MU_ERR_NOENT;
  *sptr = pop3->ackbuf;
  return 0;
}

int
mu_pop3_aget_response (mu_pop3_t pop3, char **sptr)
{
  char *p;
  
  if (pop3 == NULL)
    return EINVAL;
  if (!MU_POP3_FISSET (pop3, MU_POP3_ACK))
    return MU_ERR_NOENT;
  p = strdup (pop3->ackbuf);
  if (!p)
    return ENOMEM;
  *sptr = p;
  return 0;
}

int
mu_pop3_get_response (mu_pop3_t pop3, char *buf, size_t len, size_t *plen)
{
  size_t size;
  
  if (pop3 == NULL)
    return EINVAL;
  if (!MU_POP3_FISSET (pop3, MU_POP3_ACK))
    return MU_ERR_NOENT;
  
  if (buf)
    size = mu_cpystr (buf, pop3->ackbuf, len);
  if (plen)
    *plen = size;
  return 0;
}

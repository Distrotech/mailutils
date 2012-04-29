/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>

int
_mu_imap_seterrstr (mu_imap_t imap, const char *str, size_t len)
{
  if (!imap)
    return EINVAL;
  if (len + 1 > imap->errsize)
    {
      char *p = realloc (imap->errstr, len + 1);
      if (!p)
	return ENOMEM;
      imap->errsize = len + 1;
      imap->errstr = p;
    }
  memcpy (imap->errstr, str, len);
  imap->errstr[len] = 0;
  return 0;
}

int
_mu_imap_seterrstrz (mu_imap_t imap, const char *str)
{
  return _mu_imap_seterrstr (imap, str, strlen (str));
}

void
_mu_imap_clrerrstr (mu_imap_t imap)
{
  if (imap && imap->errstr)
    imap->errstr[0] = 0;
}
    
int
mu_imap_strerror (mu_imap_t imap, const char **pstr)
{
  if (!imap)
    {
      *pstr = "(imap not initialized)";
      return EINVAL;
    }

  if (MU_IMAP_FISSET (imap, MU_IMAP_RESP))
    {
      *pstr = imap->errstr;
      return 0;
    }
  *pstr = "(no recent reply)";
  return MU_ERR_NOENT;
}

enum mu_imap_response
mu_imap_response (mu_imap_t imap)
{
  if (!imap)
    return MU_IMAP_BAD;
  return imap->response;
}

int
mu_imap_response_code (mu_imap_t imap)
{
  if (!imap)
    return -1;
  return imap->response_code;
}



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
#include <errno.h>
#include <mailutils/sys/imap.h>

static int
_mu_imap_tag_incr (mu_imap_t imap)
{
  int i = 0;
  
  while (1)
    {
      if (++imap->tag_buf[i] <= 9)
	break;
      imap->tag_buf[i] = 0;
      if (++i == imap->tag_len)
	{
	  char *sp;
	  int *np = realloc (imap->tag_buf, imap->tag_len + 1);
	  if (!np)
	    return ENOMEM;
	  imap->tag_buf = np;
	  sp = realloc (imap->tag_str, imap->tag_len + 2);
	  if (!sp)
	    return ENOMEM;
	  imap->tag_str = sp;
	  imap->tag_len++;
	}
    }
  return 0;
}

static void
_mu_imap_tag_print (mu_imap_t imap)
{
  int i;

  for (i = 0; i < imap->tag_len; i++)
    imap->tag_str[imap->tag_len-i-1] = imap->tag_buf[i] + '0';
  imap->tag_str[i] = 0;
}

int
_mu_imap_tag_clr (mu_imap_t imap)
{
  int i;
  
  if (imap->tag_len == 0)
    {
      imap->tag_len = 2;
      imap->tag_buf = calloc (imap->tag_len, sizeof (imap->tag_buf[0]));
      if (!imap->tag_buf)
	return ENOMEM;
      imap->tag_str = calloc (imap->tag_len + 1, sizeof (imap->tag_str[0]));
      if (!imap->tag_str)
	{
	  free (imap->tag_buf);
	  return ENOMEM;
	}
    }
  for (i = 0; i < imap->tag_len; i++)
    imap->tag_buf[i] = 0;
  _mu_imap_tag_print (imap);
  return 0;
}

int
_mu_imap_tag_next (mu_imap_t imap)
{
  int status;
  status = _mu_imap_tag_incr (imap);
  if (status == 0)
    _mu_imap_tag_print (imap);
  return status;
}

int
mu_imap_tag (mu_imap_t imap, const char **ptag)
{
  if (!imap)
    return EINVAL;
  *ptag = imap->tag_str;
  return 0;
}

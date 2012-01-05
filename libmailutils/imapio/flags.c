/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/attribute.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>
#include <mailutils/imaputil.h>

static struct
{
  char *name;
  int flag;
}
_imap4_attrlist[] =
{
  { "\\Answered", MU_ATTRIBUTE_ANSWERED },
  { "\\Flagged", MU_ATTRIBUTE_FLAGGED },
  { "\\Deleted", MU_ATTRIBUTE_DELETED },
  { "\\Draft", MU_ATTRIBUTE_DRAFT },
  { "\\Seen", MU_ATTRIBUTE_SEEN|MU_ATTRIBUTE_READ },
};

static int _imap4_nattr = MU_ARRAY_SIZE (_imap4_attrlist);

int
mu_imap_flag_to_attribute (const char *item, int *attr)
{
  int i;

  if (mu_c_strcasecmp (item, "\\Recent") == 0)
    {
      *attr |= MU_ATTRIBUTE_RECENT;
      return 0;
    }
  
  for (i = 0; i < _imap4_nattr; i++)
    if (mu_c_strcasecmp (item, _imap4_attrlist[i].name) == 0)
      {
	*attr |= _imap4_attrlist[i].flag;
	return 0;
      }
  return MU_ERR_NOENT;
}

int
mu_imap_format_flags (mu_stream_t str, int flags, int include_recent)
{
  int i;
  int delim = 0;
  
  for (i = 0; i < _imap4_nattr; i++)
    if ((flags & _imap4_attrlist[i].flag) == _imap4_attrlist[i].flag)
      {
	if (delim)
	  mu_stream_printf (str, " ");
	mu_stream_printf (str, "%s", _imap4_attrlist[i].name);
	delim = 1;
      }

  if (include_recent && MU_ATTRIBUTE_IS_UNSEEN (flags))
    {
      if (delim)
	mu_stream_printf (str, " ");
      mu_stream_printf (str, "\\Recent");
    }
  
  return 0;
}


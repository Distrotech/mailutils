/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/** utility wrappers around mailutils functionality **/

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "sv.h"

#include <mailutils/attribute.h>
#include <mailutils/message.h>

#if 0
int
sv_mu_copy_debug_level (const mailbox_t from, mailbox_t to)
{
  mu_debug_t d = 0;
  size_t level;
  int rc;

  if (!from || !to)
    return EINVAL;

  if((rc = mailbox_get_debug (from, &d)))
    return rc;

  if ((mu_debug_get_level (d, &level)))
    return rc;

  if ((rc = mailbox_get_debug (to, &d)))
    return rc;

  rc = mu_debug_set_level (d, level);

  return rc;
}
#endif

int
sv_mu_mark_deleted (message_t msg, int deleted)
{
  attribute_t attr = 0;
  int rc;

  rc = message_get_attribute (msg, &attr);

  if (!rc)
    {
      if (deleted)
	rc = attribute_set_deleted (attr);
      else
	rc = attribute_unset_deleted (attr);
    }

  return rc;
}


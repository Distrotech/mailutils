/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mailutils/iterator.h>
#include <registrar0.h>

static list_t reg_list;

int
registrar_get_list (list_t *plist)
{
  if (plist == NULL)
    return EINVAL;
  if (reg_list == NULL)
    {
      int status = list_create (&reg_list);
      if (status != 0)
	return status;
    }
  *plist = reg_list;
  return 0;
}

int
record_is_scheme (record_t record, const char *scheme)
{
  if (record == NULL)
    return 0;

  /* Overload.  */
  if (record->_is_scheme)
    return record->_is_scheme (record, scheme);

  if (record->scheme && strncasecmp (record->scheme, scheme,
				    strlen (record->scheme)) == 0)
    return 1;

  return 0;
}

int
record_get_mailbox (record_t record, mailbox_entry_t *pmbox)
{
  if (record == NULL)
    return EINVAL;

  /* Overload.  */
  if (record->_get_mailbox)
    return record->_get_mailbox (record, pmbox);

  if (pmbox)
    *pmbox = record->mailbox;
  return 0;
}

int
record_get_mailer (record_t record, mailer_entry_t *pml)
{
  if (record == NULL)
    return EINVAL;

  /* Overload.  */
  if (record->_get_mailer)
    return record->_get_mailer (record, pml);

  if (pml)
    *pml = record->mailer;
  return 0;
}

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
#include <mailutils/monitor.h>
#include <registrar0.h>

/* NOTE: We will leak here since the monitor and the registrar will never
   be release.  That's ok we can leave with this, it's only done once.  */
static list_t registrar_list;
struct _monitor registrar_monitor = MU_MONITOR_INITIALIZER;

int
registrar_get_list (list_t *plist)
{
  int status = 0;
  if (plist == NULL)
    return EINVAL;
  monitor_wrlock (&registrar_monitor);
  if (registrar_list == NULL)
    status = list_create (&registrar_list);
  *plist = registrar_list;
  monitor_unlock (&registrar_monitor);
  return status;
}

int
registrar_record (record_t record)
{
  list_t list;
  registrar_get_list (&list);
  return list_append (list, record);
}

int
unregistrar_record (record_t record)
{
  list_t list;
  registrar_get_list (&list);
  list_remove (list, record);
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

  if (scheme
      && record->scheme
      && strncasecmp (record->scheme, scheme, strlen (record->scheme)) == 0)
    return 1;

  return 0;
}

int
record_set_scheme (record_t record, const char *scheme)
{
  if (record == NULL)
    return EINVAL;
  record->scheme = scheme;
  return 0;
}

int
record_set_is_scheme (record_t record, int (*_is_scheme)
		      __P ((record_t, const char *)))
{
  if (record == NULL)
    return EINVAL;
  record->_is_scheme = _is_scheme;
  return 0;
}

int
record_get_url (record_t record, int (*(*_purl)) __P ((url_t)))
{
  if (record == NULL || _purl == NULL)
    return EINVAL;
  /* Overload.  */
  if (record->_get_url)
    return record->_get_url (record, _purl);
  *_purl = record->_url;
  return 0;
}

int
record_set_url (record_t record, int (*_url) __P ((url_t)))
{
  if (record == NULL)
    return EINVAL;
  record->_url = _url;
  return 0;
}

int
record_set_get_url (record_t record, int (*_get_url)
		    __P ((record_t, int (*(*)) __P ((url_t)))))
{
  if (record == NULL)
    return EINVAL;
  record->_get_url = _get_url;
  return 0;
}

int
record_get_mailbox (record_t record, int (*(*_pmailbox)) __P ((mailbox_t)))
{
  if (record == NULL || _pmailbox == NULL)
    return EINVAL;
  /* Overload.  */
  if (record->_get_mailbox)
    return record->_get_mailbox (record, _pmailbox);
  *_pmailbox = record->_mailbox;
  return 0;
}

int
record_set_mailbox (record_t record, int (*_mailbox) __P ((mailbox_t)))
{
  if (record)
    return EINVAL;
  record->_mailbox = _mailbox;
  return 0;
}

int
record_set_get_mailbox (record_t record, int (*_get_mailbox)
			__P ((record_t, int (*(*)) __P((mailbox_t)))))
{
  if (record)
    return EINVAL;
  record->_get_mailbox = _get_mailbox;
  return 0;
}

int
record_get_mailer (record_t record, int (*(*_pmailer)) __P ((mailer_t)))
{
  if (record == NULL || _pmailer == NULL)
    return EINVAL;
  /* Overload.  */
  if (record->_get_mailer)
    return record->_get_mailer (record, _pmailer);
  *_pmailer = record->_mailer;
  return 0;
}

int
record_set_mailer (record_t record, int (*_mailer) __P ((mailer_t)))
{
  if (record)
    return EINVAL;
  record->_mailer = _mailer;
  return 0;
}

int
record_set_get_mailer (record_t record, int (*_get_mailer)
		       __P ((record_t, int (*(*)) __P ((mailer_t)))))
{
  if (record == NULL)
    return EINVAL;
  record->_get_mailer = _get_mailer;
  return 0;
}

int
record_get_folder (record_t record, int (*(*_pfolder)) __P ((folder_t)))
{
  if (record == NULL || _pfolder == NULL)
    return EINVAL;
  /* Overload.  */
  if (record->_get_folder)
    return record->_get_folder (record, _pfolder);
  *_pfolder = record->_folder;
  return 0;
}

int
record_set_folder (record_t record, int (*_folder) __P ((folder_t)))
{
  if (record == NULL)
    return EINVAL;
  record->_folder = _folder;
  return 0;
}

int
record_set_get_folder (record_t record, int (*_get_folder)
		       __P ((record_t, int (*(*)) __P ((folder_t)))))
{
  if (record == NULL)
    return EINVAL;
  record->_get_folder = _get_folder;
  return 0;
}

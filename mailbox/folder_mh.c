/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
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
# include <config.h>
#endif

#include <errno.h>

#include <folder0.h>
#include <registrar0.h>

static struct _record _mh_record =
{
  MU_MH_SCHEME,
  _url_mh_init, /* Url init.  */
  _mailbox_mh_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _folder_mh_init, /* Folder init.  */
  NULL, /* back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
record_t mh_record = &_mh_record;

int
_folder_mh_init (folder_t folder)
{
  (void)folder;
  return 0;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2003 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_MH

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

#else
#include <stdio.h>
#include <registrar0.h>
record_t mh_record = NULL;
#endif

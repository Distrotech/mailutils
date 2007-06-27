/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_MAILDIR

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <folder0.h>
#include <registrar0.h>

#include <maildir.h>
#include <mailutils/mutil.h>
#include <amd.h>

static int
_maildir_folder_init (mu_folder_t folder ARG_UNUSED)
{
  return 0;
}

static int
dir_exists (const char *name, const char *suf)
{
  struct stat st;
  char *s = maildir_mkfilename (name, suf, NULL);
      
  if (stat (s, &st) < 0)
    return 0;

  free (s);
  
  return S_ISDIR (st.st_mode);
}

static int
_maildir_is_scheme (mu_record_t record, const char *url, int flags)
{
  const char *path;
  
  if (!url || !record->scheme)
    return 0;

  if (strncmp (record->scheme, url, strlen (record->scheme)) == 0)
    return MU_FOLDER_ATTRIBUTE_ALL & flags; 

  if (mu_scheme_autodetect_p (url, &path))
    {
      /* Attemp auto-detection */
      struct stat st;
      int rc = 0;
      
      if (stat (path, &st) < 0)
	return 0; 

      if (!S_ISDIR (st.st_mode))
	return 0;

      rc |= (MU_FOLDER_ATTRIBUTE_DIRECTORY & flags);
      
      if ((flags & MU_FOLDER_ATTRIBUTE_FILE)
	  && dir_exists (path, TMPSUF)
	     && dir_exists (path, CURSUF)
 	     && dir_exists (path, NEWSUF))
        return rc | MU_FOLDER_ATTRIBUTE_FILE;
    }
  return 0;
}

/*
  MAILDIR url
  maildir:path
*/
int
_maildir_url_init (mu_url_t url)
{
  return amd_url_init (url, MU_MAILDIR_SCHEME);
}

static struct _mu_record _maildir_record =
{
  MU_MAILDIR_PRIO,
  MU_MAILDIR_SCHEME,
  _maildir_url_init, /* Url init.  */
  _mailbox_maildir_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _maildir_folder_init, /* Folder init.  */
  NULL, /* back pointer.  */
  _maildir_is_scheme, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
mu_record_t mu_maildir_record = &_maildir_record;

#else
#include <stdio.h>
#include <registrar0.h>
mu_record_t mu_maildir_record = NULL;
#endif

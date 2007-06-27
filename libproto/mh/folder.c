/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2003, 2004, 2005,
   2007 Free Software Foundation, Inc.

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

#ifdef ENABLE_MH

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include <url0.h>
#include <folder0.h>
#include <registrar0.h>
#include <amd.h>
#include <mailutils/mutil.h>

static int
_mh_folder_init (mu_folder_t folder ARG_UNUSED)
{
  return 0;
}

/* Check if NAME is a valid MH message name */
static int
mh_message_name_p (const char *name)
{
  for ( ; *name; name++)
    if (!isdigit (*name))
      return 0;
  return 1;
}

/* Check if directory NAME is a valid MH folder directory */
static int
mh_dir_p (const char *name)
{
  DIR *dir;
  struct dirent *entry;
  int result = 0;
  
  dir = opendir (name);
  if (!dir)
    return 1; /* Maybe yes */

  while (!result && (entry = readdir (dir)))
    {
      switch (entry->d_name[0])
	{
	case '.':
	  result = strcmp (entry->d_name, ".mh_sequences") == 0;
	  break;

	case ',':
	  result = mh_message_name_p (entry->d_name + 1);
	  break;

	default:
	  result = mh_message_name_p (entry->d_name);
	  break;
	}
    }
  closedir (dir);
  return result;
}

static int
_mh_is_scheme (mu_record_t record, const char *url, int flags)
{
  const char *path;
  int rc = 0;
  
  if (!url || !record->scheme)
    return 0;

  if (strncmp (record->scheme, url, strlen (record->scheme)) == 0)
    return MU_FOLDER_ATTRIBUTE_ALL & flags;

  if (mu_scheme_autodetect_p (url, &path))
    {
      /* Attemp auto-detection */
      struct stat st;
      
      if (stat (path, &st) < 0)
	return 0; /* mu_mailbox_open will complain*/

      if (!S_ISDIR (st.st_mode))
	return 0;

      rc |= (MU_FOLDER_ATTRIBUTE_DIRECTORY & flags);
      
      if ((flags & MU_FOLDER_ATTRIBUTE_FILE) && mh_dir_p (path))
        return rc | MU_FOLDER_ATTRIBUTE_FILE;
    }

  return 0;
}

/*
  MH url
  mh:path
*/
static int
_mh_url_init (mu_url_t url)
{
  return amd_url_init (url, MU_MH_SCHEME);
}

static struct _mu_record _mh_record =
{
  MU_MH_PRIO,
  MU_MH_SCHEME,
  _mh_url_init, /* Url init.  */
  _mailbox_mh_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _mh_folder_init, /* Folder init.  */
  NULL, /* back pointer.  */
  _mh_is_scheme, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
mu_record_t mu_mh_record = &_mh_record;

#else
#include <stdio.h>
#include <registrar0.h>
mu_record_t mu_mh_record = NULL;
#endif

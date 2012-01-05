/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2003-2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

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

#ifdef ENABLE_MH

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include <mailutils/sys/url.h>
#include <mailutils/sys/folder.h>
#include <mailutils/sys/registrar.h>
#include <mailutils/sys/amd.h>
#include <mailutils/util.h>
#include <mailutils/cctype.h>

/* Check if NAME is a valid MH message name */
static int
mh_message_name_p (const char *name)
{
  for ( ; *name; name++)
    if (!mu_isdigit (*name))
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
_mh_is_scheme (mu_record_t record, mu_url_t url, int flags)
{
  int rc = 0;
  int scheme_matched = mu_url_is_scheme (url, record->scheme);
  
  if (scheme_matched || mu_scheme_autodetect_p (url))
    {
      /* Attempt auto-detection */
      const char *path;
      struct stat st;
      
      if (mu_url_sget_path (url, &path))
        return 0;

      if (stat (path, &st) < 0)
	{
	  if (errno == ENOENT && scheme_matched)
	    return MU_FOLDER_ATTRIBUTE_ALL & flags; 
	  return 0; /* mu_mailbox_open will complain*/
	}
      
      if (!S_ISDIR (st.st_mode))
	return 0;

      if (scheme_matched)
	rc = MU_FOLDER_ATTRIBUTE_ALL;
      else
	{
	  rc |= MU_FOLDER_ATTRIBUTE_DIRECTORY;
      
	  if ((flags & MU_FOLDER_ATTRIBUTE_FILE) && mh_dir_p (path))
	    rc |= MU_FOLDER_ATTRIBUTE_FILE;
	}
    }

  return rc & flags;
}

static int
_mh_list_p (mu_record_t record, const char *name, int flags MU_ARG_UNUSED)
{
  if (name[0] == ',' ||
      ((strlen (name) > 3) &&
       (memcmp (name, ".mh", 3) == 0 || memcmp (name, ".mu", 3) == 0)))
    return 0;

  for (; *name; name++)
    if (!mu_isdigit (*name))
      return 1;
  return 0;
}

static struct _mu_record _mh_record =
{
  MU_MH_PRIO,
  MU_MH_SCHEME,
  MU_RECORD_LOCAL,
  MU_URL_SCHEME | MU_URL_PATH | MU_URL_PARAM,
  MU_URL_PATH,
  mu_url_expand_path, /* Url init.  */
  _mailbox_mh_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _mu_fsfolder_init, /* Folder init.  */
  NULL, /* back pointer.  */
  _mh_is_scheme, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL, /* _get_folder method.  */
  _mh_list_p
};
mu_record_t mu_mh_record = &_mh_record;

#else
#include <stdio.h>
#include <mailutils/sys/registrar.h>
mu_record_t mu_mh_record = NULL;
#endif

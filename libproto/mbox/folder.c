/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003-2008, 2010-2012 Free Software
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

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>

#include <mailutils/sys/folder.h>
#include <mailutils/sys/registrar.h>

#include <mailutils/auth.h>
#include <mailutils/url.h>
#include <mailutils/stream.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/debug.h>

/* We export url parsing and the initialisation of
   the mailbox, via the register entry/record.  */

static int
_mbox_is_scheme (mu_record_t record, mu_url_t url, int flags)
{
  int rc = 0;
  int scheme_matched = mu_url_is_scheme (url, record->scheme);
  if (scheme_matched || mu_scheme_autodetect_p (url))
    {
      struct stat st;
      const char *path;

      mu_url_sget_path (url, &path);
      if (stat (path, &st) < 0)
	{
	  if (errno == ENOENT)
	    {
	      if (scheme_matched)
		return MU_FOLDER_ATTRIBUTE_FILE & flags;
	    }
	  return 0;
	}
      
      if (S_ISREG (st.st_mode) || S_ISCHR (st.st_mode))
	{
	  if (st.st_size == 0)
	    {
	      rc |= MU_FOLDER_ATTRIBUTE_FILE;
	    }
	  else if (flags & MU_FOLDER_ATTRIBUTE_FILE)
	    {
#if 0
	      /* FIXME: This effectively sieves out all non-mailbox files,
		 but it makes mu_folder_enumerate crawl, which is
		 intolerable for imap4d LIST command. */
	      int fd = open (path, O_RDONLY);
	      if (fd != -1)
		{
		  char buf[5];
		  if (read (fd, buf, 5) == 5)
		    if (memcmp (buf, "From ", 5) == 0)
		      rc |= MU_FOLDER_ATTRIBUTE_FILE;
		  close (fd);
		}
#else
	      rc |= MU_FOLDER_ATTRIBUTE_FILE;
#endif
	    }
	}
	  
      if ((flags & MU_FOLDER_ATTRIBUTE_DIRECTORY)
	  && S_ISDIR (st.st_mode))
	rc |= MU_FOLDER_ATTRIBUTE_DIRECTORY;
    }
  return rc;
}

static struct _mu_record _mbox_record =
{
  MU_MBOX_PRIO,
  MU_MBOX_SCHEME,
  MU_RECORD_LOCAL,
  MU_URL_SCHEME | MU_URL_PATH | MU_URL_PARAM,
  MU_URL_PATH,
  mu_url_expand_path, /* URL init.  */
  _mailbox_mbox_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _mu_fsfolder_init, /* Folder init.  */
  NULL, /* No need for an back pointer.  */
  _mbox_is_scheme, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
mu_record_t mu_mbox_record = &_mbox_record;



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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <mailbox0.h>
#include <registrar0.h>

/*
  Caveat there is no specific URL for file mailbox or simple path name,
    <path_name>
    file:<path_name>

  It would be preferrable to use :
    maildir:<path>
    unix:<path>
    mmdf:<path>
  This would eliminate heuristic discovery that would turn
  out to be wrong.
*/

int
_mailbox_file_init (mailbox_t mbox)
{
  struct stat st;
  size_t len = 0;
  char *path;
  int status;

  status = url_get_path (mbox->url, NULL, 0, &len);
  if (status != 0)
    return status;
  path = calloc (len + 1, sizeof (char));
  if (path == NULL)
    return ENOMEM;
  status = url_get_path (mbox->url, path, len + 1, NULL);
  if (status != 0)
    {
      free (path);
      return status;
    }

  /* Sigh, if they want to creat ??? they should know the type of ???
     What is the best course of action ? For the default is mbox if the
     file does not exist.  */
  if (stat (path, &st) < 0)
    {
      status = _mailbox_mbox_init (mbox);
    }
  else if (S_ISREG (st.st_mode))
    {
      /*
	FIXME: We should do an open() and try
	to do a better reconnaissance of the type,
	maybe MMDF.  For now assume Unix MBox */
      status = _mailbox_mbox_init (mbox);
    }
  /* Is that true ?  Are all directories Maildir ?? */
  else if (S_ISDIR (st.st_mode))
    {
      /*status = _mailbox_maildir_init (mbox);*/
      status = EINVAL;
    }

  free (path);
  return status;
}

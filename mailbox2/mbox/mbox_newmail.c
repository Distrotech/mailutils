/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/stat.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_has_newmail (mbox_t mbox)
{
  int newmail = 0;

  mbox_debug_print (mbox, "has_newmail");

  /* If the modification time is greater then the access time, the file has
     been modified since the last time it was accessed.  This typically means
     new mail or someone tempered with the mailbox.  */
  if (mbox && mbox->carrier)
    {
      int fd = -1;
      if (stream_get_fd (mbox->carrier, &fd) == 0)
	{
	  struct stat statbuf;
	  if (fstat (fd, &statbuf) == 0)
	    {
	      if (difftime (statbuf.st_mtime, statbuf.st_atime) > 0)
		newmail = 1;
	    }
	}
    }
  return newmail;
}

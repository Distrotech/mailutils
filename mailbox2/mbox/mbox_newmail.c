/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
# include <config.h>
#endif

#include <sys/stat.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_changed_on_disk (mbox_t mbox)
{
  int changed = 0;
  /* Check if the mtime stamp changed, random modifications can give
     us back the same size.  */
  if (mbox->carrier)
    {
      int fd = -1;
      if (stream_get_fd (mbox->carrier, &fd) == 0)
	{
	  struct stat statbuf;
	  if (fstat (fd, &statbuf) == 0)
	    {
	      if (difftime (mbox->mtime, statbuf.st_mtime) != 0)
		changed = 1;
	    }
	}
    }
  return changed;
}

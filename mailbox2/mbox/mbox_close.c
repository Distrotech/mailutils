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

#include <stdlib.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_close (mbox_t mbox)
{
  size_t i;

  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* Make sure that we do not hold any file locking.  */
  if (mbox->lockfile)
    {
      lockfile_unlock (mbox->lockfile);
      lockfile_destroy (&mbox->lockfile);
    }

  /* Before closing we need to remove all the umesages
     - to reclaim the memory
     - to prepare for another scan.  */
  for (i = 0; i < mbox->umessages_count; i++)
    {
      mbox_message_t mum = mbox->umessages[i];
      /* Destroy the attach messages.  */
      if (mum)
	{
	  mbox_hcache_free (mbox, i + 1);
	  if (mum->header.stream)
	    stream_destroy (&(mum->header.stream));
	  if (mum->body.stream)
	    stream_destroy (&(mum->body.stream));
	  if (mum->separator)
	    free (mum->separator);
	  if (mum->attribute)
	    attribute_destroy (&mum->attribute);
	  free (mum);
	}
    }
  if (mbox->umessages)
    free (mbox->umessages);
  mbox->umessages = NULL;
  mbox->umessages_count = 0;
  mbox->size = 0;
  mbox->mtime = 0;
  mbox->uidvalidity = 0;
  mbox->uidnext = 0;
  if (mbox->filename)
    free (mbox->filename);
  mbox->filename = NULL;
  return stream_close (mbox->carrier);
}

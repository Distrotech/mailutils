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

#include <stdlib.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_close (mbox_t mbox)
{
  size_t i;

  mbox_debug_print (mbox, "close");

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
      /* Destroy the attach streams and attribute.  */
      if (mbox->umessages[i])
	mbox_release_msg (mbox, i + 1);
    }
  if (mbox->umessages)
    free (mbox->umessages);
  mbox->umessages = NULL;
  mbox->umessages_count = 0;
  mbox->messages_count = 0;
  mbox->size = 0;
  mbox->uidvalidity = 0;
  mbox->uidnext = 0;
  if (mbox->filename)
    free (mbox->filename);
  mbox->filename = NULL;

  return stream_close (mbox->carrier);
}

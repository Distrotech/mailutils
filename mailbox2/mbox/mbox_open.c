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

#include <string.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_open (mbox_t mbox, const char *filename, int flags)
{
  int status = 0;
  char from[12];
  size_t n = 0;

  mbox_debug_print (mbox, "open(%s,%d)", (filename) ? filename : "", flags);

  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (mbox->carrier == NULL)
    {
      stream_t carrier;
      status = mbox_get_carrier (mbox, &carrier);
      if (status != 0)
	return status;
    }
  else
    mbox_close (mbox);

  status = stream_open (mbox->carrier, filename, 0, flags);
  if (status != 0)
    return status;

  /* We need to be able to seek on the stream.  */
  if (!stream_is_seekable (mbox->carrier))
    return MU_ERROR_NOT_SUPPORTED;

  /* Check if it is indeed a mbox format.  */
  stream_readline (mbox->carrier, from, sizeof from, 0, &n);
  if (status != 0)
    return status;

  /* Empty file is Ok.  */
  if (n)
    {
      status = strncmp (from, "From ", 5);
      if (status != 0)
	return MU_ERROR_NOT_SUPPORTED;
    }

  /* Give an appropriate way to file lock.  */
  /* FIXME: use dotlock external program: we may not be setgid.  */
  status = lockfile_dotlock_create (&(mbox->lockfile), filename);
  mbox->filename = strdup (filename);
  return status;
}

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

/* Save the uidvalidity
   - if it is an empty mbox in the first message append
   - if for the first message the uidvalidity is not the same
   from the mbox->uidvalidity.

   - strip X-IMAPBASE, X-UID
   - add X-UID base on mbox->uidnext.

   - mangle any leading "From " in the body to ">From "
*/

int
mbox_append (mbox_t mbox, const char *sep, stream_t stream)
{
  if (mbox == NULL || stream == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  (void)sep;
  return 0;
}

int
mbox_append_hb (mbox_t mbox, const char *sep, stream_t hstream,
		stream_t bstream)
{
  if (mbox == NULL || hstream == NULL || bstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  (void)sep;
  return 0;
}

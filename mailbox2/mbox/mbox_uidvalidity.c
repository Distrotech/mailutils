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

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_get_uidvalidity (mbox_t mbox, unsigned long *puidvalidity)
{
  if (mbox == NULL || puidvalidity == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* If we did not start a scanning yet do it now.  */
  if (mbox->messages_count == 0)
    {
      int status = mbox_scan (mbox, 0, NULL, 0);
      if (status != 0)
        return status;
    }
  *puidvalidity = mbox->uidvalidity;
  return 0;
}

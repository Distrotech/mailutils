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

#include <stdlib.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_get_hstream (mbox_t mbox, unsigned int msgno, stream_t *pstream)
{
  int status = 0;

  mbox_debug_print (mbox, "get_hstream(%u)", msgno);
  if (mbox == NULL || msgno == 0 || pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->messages_count)
    return MU_ERROR_INVALID_PARAMETER;

  msgno--;
  if (mbox->umessages[msgno]->header.stream)
    {
      stream_ref (mbox->umessages[msgno]->header.stream);
      *pstream = mbox->umessages[msgno]->header.stream;
    }
  else
    {
      status = stream_mbox_create (pstream, mbox, msgno + 1, 1);
      if (status == 0)
	mbox->umessages[msgno]->header.stream = *pstream;
    }
  return status;
}

int
mbox_set_hstream (mbox_t mbox, unsigned int msgno, stream_t stream)
{
  if (mbox == NULL || msgno == 0)
    return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->umessages_count)
    return MU_ERROR_INVALID_PARAMETER;

  if (mbox->umessages[msgno - 1]->header.stream)
    mbox_release_hstream (mbox, msgno);

  mbox->umessages[msgno - 1]->attr_flags |= MU_MBOX_HSTREAM_SET;
  mbox->umessages[msgno - 1]->attr_flags |= MU_ATTRIBUTE_MODIFIED;
  mbox->umessages[msgno - 1]->header.stream = stream;
  return 0;
}

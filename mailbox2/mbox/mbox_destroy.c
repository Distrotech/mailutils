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

#include <mailutils/sys/mbox.h>

void
mbox_release_separator (mbox_t mbox, unsigned int msgno)
{
  if (mbox == NULL || msgno == 0)
    return;

  if (msgno > mbox->umessages_count)
    return;

  if (mbox->umessages[msgno - 1]->separator)
    free (mbox->umessages[msgno - 1]->separator);
  mbox->umessages[msgno - 1]->separator = NULL;
}

void
mbox_release_attribute (mbox_t mbox, unsigned int msgno)
{
  if (mbox == NULL || msgno == 0)
    return;

  if (msgno > mbox->umessages_count)
    return;

  if (mbox->umessages[msgno - 1]->attribute)
    _attribute_mbox_dtor (mbox->umessages[msgno - 1]->attribute);
  mbox->umessages[msgno - 1]->attribute = NULL;
}

void
mbox_release_hstream (mbox_t mbox, unsigned int msgno)
{
  if (mbox == NULL || msgno == 0)
    return;

  if (msgno > mbox->umessages_count)
    return;

  if (mbox->umessages[msgno - 1]->header.stream)
    {
      if (mbox->umessages[msgno - 1]->attr_userflags & MU_MBOX_HSTREAM_SET)
	{
	  stream_destroy (&mbox->umessages[msgno - 1]->header.stream);
	  mbox->umessages[msgno - 1]->attr_userflags &= ~MU_MBOX_HSTREAM_SET;
	}
      else
	_stream_mbox_dtor (mbox->umessages[msgno - 1]->header.stream);
    }
  mbox->umessages[msgno - 1]->header.stream = NULL;
}

void
mbox_release_bstream (mbox_t mbox, unsigned int msgno)
{
  if (mbox == NULL || msgno == 0)
    return;

  if (msgno > mbox->umessages_count)
    return;

  if (mbox->umessages[msgno - 1]->body.stream)
    {
      if (mbox->umessages[msgno - 1]->attr_userflags & MU_MBOX_BSTREAM_SET)
	{
	  stream_destroy (&mbox->umessages[msgno - 1]->body.stream);
	  mbox->umessages[msgno - 1]->attr_userflags &= ~MU_MBOX_BSTREAM_SET;
	}
      else
	_stream_mbox_dtor (mbox->umessages[msgno - 1]->body.stream);
    }
  mbox->umessages[msgno - 1]->body.stream = NULL;
}

void
mbox_release_msg (mbox_t mbox, unsigned int msgno)
{
  if (mbox == NULL || msgno == 0)
    return;

  if (msgno > mbox->umessages_count)
    return;

  mbox_release_hcache (mbox, msgno);
  mbox_release_separator (mbox, msgno);
  mbox_release_attribute (mbox, msgno);
  mbox_release_hstream (mbox, msgno);
  mbox_release_bstream (mbox, msgno);
  free (mbox->umessages[msgno - 1]);
  mbox->umessages[msgno - 1] = NULL;
}

void
mbox_destroy (mbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mbox_t mbox = *pmbox;
      mbox_debug_print (mbox, "destroy");
      /* _close will release all the messages.  */
      mbox_close (mbox);
      if (mbox->carrier)
	stream_destroy (&mbox->carrier);
      if (mbox->debug)
	mu_debug_destroy (&mbox->debug);
      mbox_release_hcache (mbox, 0);
      free (mbox);
    }
}

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
#include <mailutils/sys/message.h>

int
message_ref (message_t msg)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->ref (msg);
}

void
message_destroy (message_t *pmsg)
{
  if (pmsg && *pmsg)
    {
      message_t msg = *pmsg;
      if (msg->vtable && msg->vtable->destroy)
	msg->vtable->destroy (pmsg);
      *pmsg = NULL;
    }
}

int
message_is_modified (message_t msg)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->is_modified == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->is_modified (msg);
}

int
message_clear_modified (message_t msg)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->clear_modified == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->clear_modified (msg);
}

int
message_get_mailbox (message_t msg, mailbox_t *mbox)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_mailbox == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_mailbox (msg, mbox);
}

int
message_get_envelope (message_t msg, envelope_t *envelope)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_envelope == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_envelope (msg, envelope);
}

int
message_get_header (message_t msg, header_t *header)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_header == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_header (msg, header);
}

int
message_get_body (message_t msg, body_t *body)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_body == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_body (msg, body);
}

int
message_get_attribute (message_t msg, attribute_t *attribute)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_attribute == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_attribute (msg, attribute);
}

int
message_get_stream (message_t msg, stream_t *stream)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_stream == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_stream (msg, stream);
}

int
message_get_property (message_t msg, property_t *property)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_property == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_property (msg, property);
}

int
message_is_multipart (message_t msg, int *ismulti)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->is_multipart == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->is_multipart (msg, ismulti);
}

int
message_get_size (message_t msg, size_t *size)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_size == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_size (msg, size);
}

int
message_get_lines (message_t msg, size_t *lines)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_lines == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_lines (msg, lines);
}

int
message_get_num_parts (message_t msg, size_t *nparts)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_num_parts == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_num_parts (msg, nparts);
}

int
message_get_part (message_t msg, size_t partno, message_t *submsg)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_part == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_part (msg, partno, submsg);
}

int
message_get_uidl (message_t msg, char *uidl, size_t len, size_t *plen)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_uidl == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_uidl (msg, uidl, len, plen);
}

int
message_get_uid (message_t msg, size_t *uid)
{
  if (msg == NULL || msg->vtable == NULL
      || msg->vtable->get_uid == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return msg->vtable->get_uid (msg, uid);
}

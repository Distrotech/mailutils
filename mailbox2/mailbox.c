/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mailutils/error.h>
#include <mailutils/sys/mailbox.h>

int
mailbox_ref (mailbox_t mailbox)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->ref (mailbox);
}

void
mailbox_destroy (mailbox_t *pmailbox)
{
  if (pmailbox && *pmailbox)
    {
      mailbox_t mailbox = *pmailbox;
      if (mailbox->vtable && mailbox->vtable->destroy)
	mailbox->vtable->destroy (pmailbox);
      *pmailbox = NULL;
    }
}

int
mailbox_get_folder (mailbox_t mailbox, folder_t *folder)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_folder == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_folder (mailbox, folder);
}

int
mailbox_open (mailbox_t mailbox, int flag)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->open == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->open (mailbox, flag);
}

int
mailbox_close (mailbox_t mailbox)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->close == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->close (mailbox);
}

int
mailbox_uidvalidity (mailbox_t mailbox, unsigned long *uidvalidity)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->uidvalidity == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->uidvalidity (mailbox, uidvalidity);
}

int
mailbox_uidnext (mailbox_t mailbox, size_t *uidnext)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->uidnext == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->uidnext (mailbox, uidnext);
}

/* Messages.  */
int
mailbox_get_message (mailbox_t mailbox, size_t msgno, message_t *msg)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_message == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_message (mailbox, msgno, msg);
}

int
mailbox_append_message  (mailbox_t mailbox, message_t msg)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->append_message == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->append_message (mailbox, msg);
}

int
mailbox_messages_count (mailbox_t mailbox, size_t *count)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->messages_count == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->messages_count (mailbox, count);
}

int
mailbox_messages_recent (mailbox_t mailbox, size_t *recent)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->messages_recent == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->messages_recent (mailbox, recent);
}

int
mailbox_messages_unseen (mailbox_t mailbox, size_t *unseen)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->messages_unseen == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->messages_unseen (mailbox, unseen);
}

int
mailbox_expunge (mailbox_t mailbox)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->expunge == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->expunge (mailbox);
}

int
mailbox_save_attributes (mailbox_t mailbox)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->save_attributes == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->save_attributes (mailbox);
}

/* Update and scanning.  */
int
mailbox_get_size (mailbox_t mailbox, off_t *size)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_size == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_size (mailbox, size);
}

int
mailbox_is_updated (mailbox_t mailbox)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->is_updated == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->is_updated (mailbox);
}

int
mailbox_scan (mailbox_t mailbox, size_t no, size_t *count)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->scan == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->scan (mailbox, no, count);
}

/* Mailbox Stream.  */
int
mailbox_get_stream (mailbox_t mailbox, stream_t *stream)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_stream == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_stream (mailbox, stream);
}

/* Authentication.  */
int
mailbox_get_authority (mailbox_t mailbox, authority_t *authority)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_authority == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_authority (mailbox, authority);
}

int
mailbox_set_authority (mailbox_t mailbox, authority_t authority)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->set_authority == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->set_authority (mailbox, authority);
}

/* Property.  */
int
mailbox_get_property (mailbox_t mailbox, property_t *property)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_property == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_property (mailbox, property);
}

/* URL.  */
int
mailbox_get_url (mailbox_t mailbox, url_t *url)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_url == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_url (mailbox, url);
}

/* For any debuging */
int
mailbox_get_debug (mailbox_t mailbox, mu_debug_t *debug)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_debug == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_debug (mailbox, debug);
}

int
mailbox_set_debug (mailbox_t mailbox, mu_debug_t debug)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->set_debug == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->set_debug (mailbox, debug);
}

/* Events.  */
int
mailbox_get_observable (mailbox_t mailbox, observable_t *observable)
{
  if (mailbox == NULL || mailbox->vtable == NULL
      || mailbox->vtable->get_observable == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return mailbox->vtable->get_observable (mailbox, observable);
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/iterator.h>
#include <mailutils/message.h>
#include <mailutils/attribute.h>

#include <mailutils/sys/mailbox.h>

struct mailbox_iterator
{
  mu_mailbox_t mbx;
  size_t idx;
  int backwards;
};

static int
mbx_first (void *owner)
{
  struct mailbox_iterator *itr = owner;
  if (itr->backwards)
    return mu_mailbox_messages_count (itr->mbx, &itr->idx);
  else
    itr->idx = 1;
  return 0;
}

static int
mbx_next (void *owner)
{
  struct mailbox_iterator *itr = owner;

  if (itr->backwards)
    {
      if (itr->idx)
	--itr->idx;
    }
  else
    itr->idx++;
  return 0;
}

static int
mbx_getitem (void *owner, void **pret, const void **pkey)
{
  struct mailbox_iterator *itr = owner;
  size_t count;
  int rc;
  
  rc = mu_mailbox_messages_count (itr->mbx, &count);
  if (rc)
    return rc;
  if (itr->idx > count)
    return MU_ERR_NOENT;
  rc = mu_mailbox_get_message (itr->mbx, itr->idx, (mu_message_t*)pret);
  if (rc == 0 && pkey)
    *pkey = NULL; /* FIXME: Perhaps return UIDL or other unique id? */
  return rc;
}

static int
mbx_finished_p (void *owner)
{
  struct mailbox_iterator *itr = owner;
  
  if (itr->backwards)
    return itr->idx == 0;
  else
    {
      size_t count;
      
      if (mu_mailbox_messages_count (itr->mbx, &count))
	return 1;
      return itr->idx > count;
    }
}

static int
mbx_destroy (mu_iterator_t iterator, void *data)
{
  struct mailbox_iterator *itr = data;

  mu_iterator_detach (&itr->mbx->iterator, iterator);
  free (data);
  return 0;
}

static int
mbx_delitem (void *owner, void *item)
{
  void *ptr;

  if (mbx_getitem (owner, &ptr, NULL))
    return 0;
  return ptr == item ? MU_ITR_DELITEM_NEXT : MU_ITR_DELITEM_NOTHING;
  /* FIXME: is it ok? */
}

static int
mbx_data_dup (void **ptr, void *owner)
{
  struct mailbox_iterator *itr = owner;

  *ptr = malloc (sizeof (struct mailbox_iterator));
  if (*ptr == NULL)
    return ENOMEM;
  memcpy (*ptr, owner, sizeof (struct mailbox_iterator));
  mu_iterator_attach (&itr->mbx->iterator, *ptr);
  return 0;
}

static int
mbx_itrctl (void *owner, enum mu_itrctl_req req, void *arg)
{
  int rc;
  struct mailbox_iterator *itr = owner;
  mu_message_t msg;
  mu_attribute_t attr;
  
  if (itr->idx == 0)
    return MU_ERR_NOENT;
  switch (req)
    {
    case mu_itrctl_tell:
      *(size_t*)arg = itr->idx;
      break;

    case mu_itrctl_delete:
      rc = mu_mailbox_get_message (itr->mbx, itr->idx, &msg);
      if (rc)
	return rc;
      rc = mu_message_get_attribute (msg, &attr);
      if (rc)
	return rc;
      rc = mu_attribute_set_deleted (attr);
      if (rc)
	return rc;
      break;

    case mu_itrctl_qry_direction:
      if (!arg)
	return EINVAL;
      else
	*(int*)arg = itr->backwards;
      break;

    case mu_itrctl_set_direction:
      if (!arg)
	return EINVAL;
      else	
	itr->backwards = !!*(int*)arg;
      break;
      
    default:
      return ENOSYS;
    }
  return 0;
}


int
mu_mailbox_get_iterator (mu_mailbox_t mbx, mu_iterator_t *piterator)
{
  mu_iterator_t iterator;
  int status;
  struct mailbox_iterator *itr;

  if (!mbx)
    return EINVAL;

  itr = calloc (1, sizeof *itr);
  if (!itr)
    return ENOMEM;
  itr->mbx = mbx;
  itr->idx = 1;
  itr->backwards = 0;
  
  status = mu_iterator_create (&iterator, itr);
  if (status)
    {
      free (itr);
      return status;
    }

  mu_iterator_set_first (iterator, mbx_first);
  mu_iterator_set_next (iterator, mbx_next);
  mu_iterator_set_getitem (iterator, mbx_getitem);
  mu_iterator_set_finished_p (iterator, mbx_finished_p);
  mu_iterator_set_delitem (iterator, mbx_delitem);
  mu_iterator_set_destroy (iterator, mbx_destroy);
  mu_iterator_set_dup (iterator, mbx_data_dup);
  mu_iterator_set_itrctl (iterator, mbx_itrctl);

  mu_iterator_attach (&mbx->iterator, iterator);

  *piterator = iterator;
  return 0;
}





  


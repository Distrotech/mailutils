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
#include <string.h>

#include <mailutils/error.h>
#include <mailutils/refcount.h>
#include <mailutils/sys/attribute.h>
#include <mailutils/sys/mbox.h>

struct _attribute_mbox
{
  struct _attribute base;
  mu_refcount_t refcount;
  mbox_t mbox;
  unsigned int msgno;
};

static int
_attribute_mbox_ref (attribute_t attribute)
{
  struct _attribute_mbox *ma = (struct _attribute_mbox *)attribute;
  return mu_refcount_inc (ma->refcount);
}

static void
_attribute_mbox_destroy (attribute_t *pattribute)
{
  struct _attribute_mbox *ma = (struct _attribute_mbox *)*pattribute;
  if (mu_refcount_dec (ma->refcount) == 0)
    {
      if (ma->msgno <= ma->mbox->messages_count)
	{
	  /* If it is the attribute save in the mailbox structure.  */
	  if (ma == (struct _attribute_mbox *)(ma->mbox->umessages[ma->msgno - 1]->attribute))
	    ma->mbox->umessages[ma->msgno - 1]->attribute = NULL;
	  mu_refcount_destroy (&ma->refcount);
	  free (ma);
	}
    }
}

static int
_attribute_mbox_get_flags (attribute_t attribute, int *pflags)
{
  struct _attribute_mbox *ma = (struct _attribute_mbox *)attribute;
  if (pflags)
    {
      if (ma->msgno <= ma->mbox->messages_count)
	{
	  *pflags = ma->mbox->umessages[ma->msgno - 1]->attr_flags;
	}
    }
  return 0;
}

static int
_attribute_mbox_set_flags (attribute_t attribute, int flags)
{
  struct _attribute_mbox *ma = (struct _attribute_mbox *)attribute;
  if (ma->msgno <= ma->mbox->messages_count)
    {
      ma->mbox->umessages[ma->msgno - 1]->attr_flags |= (flags | MU_ATTRIBUTE_MODIFIED);
    }
  return 0;
}

static int
_attribute_mbox_unset_flags (attribute_t attribute, int flags)
{
  struct _attribute_mbox *ma = (struct _attribute_mbox *)attribute;
  if (ma->msgno <= ma->mbox->messages_count)
    {
      ma->mbox->umessages[ma->msgno - 1]->attr_flags &= ~flags;
      /* If Modified was being unset do not reset it.  */
      if (!(flags & MU_ATTRIBUTE_MODIFIED))
	ma->mbox->umessages[ma->msgno - 1]->attr_flags |= MU_ATTRIBUTE_MODIFIED;
    }
  return 0;
}

static int
_attribute_mbox_clear_flags (attribute_t attribute)
{
  struct _attribute_mbox *ma = (struct _attribute_mbox *)attribute;
  if (ma->msgno <= ma->mbox->messages_count)
    {
      ma->mbox->umessages[ma->msgno - 1]->attr_flags = 0;
    }
  return 0;
}

static struct _attribute_vtable _attribute_mbox_vtable =
{
  _attribute_mbox_ref,
  _attribute_mbox_destroy,

  _attribute_mbox_get_flags,
  _attribute_mbox_set_flags,
  _attribute_mbox_unset_flags,
  _attribute_mbox_clear_flags
};

static int
_attribute_mbox_ctor (struct _attribute_mbox *ma, mbox_t mbox,
			  unsigned int msgno)
{
  mu_refcount_create (&(ma->refcount));
  if (ma->refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  ma->mbox = mbox;
  ma->msgno = msgno;
  ma->base.vtable = &_attribute_mbox_vtable;
  return 0;
}

/*
static int
_attribute_mbox_dtor (struct _attribute_mbox *ma)
{
}
*/

int
attribute_mbox_create (attribute_t *pattribute, mbox_t mbox,
			   unsigned int msgno)
{
  struct _attribute_mbox *ma;
  int status;
  attribute_t attribute;
  char buf[128];

  /* Get the attribute from the status field.  */
  *buf = '\0';
  status = mbox_header_get_value (mbox, msgno, "Status", buf, sizeof buf, 0);
  if (status != 0)
    return status;
  status = attribute_status_create (&attribute, buf);
  if (status != 0)
    return status;
  attribute_get_flags (attribute, &mbox->umessages[msgno - 1]->attr_flags);
  attribute_destroy (&attribute);

  ma = calloc (1, sizeof *ma);
  if (ma == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _attribute_mbox_ctor (ma, mbox, msgno);
  if (status != 0)
    {
      free (ma);
      return status;
    }
  *pattribute = &ma->base;
  return 0;
}

int
mbox_get_attribute (mbox_t mbox, unsigned int msgno, attribute_t *pattribute)
{
  int status = 0;
  if (mbox == NULL || msgno == 0 || pattribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->messages_count)
    return MU_ERROR_INVALID_PARAMETER;

  msgno--;
  if (mbox->umessages[msgno]->attribute)
    {
      attribute_ref (mbox->umessages[msgno]->attribute);
      *pattribute = mbox->umessages[msgno]->attribute;
    }
  else
    {
      status = attribute_mbox_create (pattribute, mbox, msgno + 1);
      if (status == 0)
	mbox->umessages[msgno]->attribute = *pattribute;
    }
  return status;
}

int
mbox_attribute_to_status (attribute_t attribute, char *buf, size_t buflen,
			  size_t *pn)
{
  char Status[32];
  char a[8];
  size_t i;

  *Status = *a = '\0';

  if (attribute)
    {
      if (attribute_is_read (attribute))
	strcat (a, "R");
      if (attribute_is_seen (attribute))
	strcat (a, "O");
      if (attribute_is_answered (attribute))
	strcat (a, "A");
      if (attribute_is_deleted (attribute))
	strcat (a, "d");
      if (attribute_is_flagged (attribute))
	strcat (a, "F");
    }

  if (*a != '\0')
    {
      strcpy (Status, "Status: ");
      strcat (Status, a);
      strcat (Status, "\n");
    }

  if (buf && buflen)
    {
      *buf = '\0';
      strncpy (buf, Status, buflen - 1);
      buf[buflen - 1] = '\0';
      i = strlen (buf);
    }
  else
    i = strlen (Status);

  if (pn)
    *pn = i;
  return 0;
}

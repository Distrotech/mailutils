/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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
#include <config.h>
#endif

#include <registrar0.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
  Builtin mailbox types.
  A circular list is use for the builtin.
  Proper locking is not done when accessing the list.
  FIXME: not thread-safe. */

static struct _registrar registrar [] = {
  { NULL, NULL, 0, &registrar[1] }, /* sentinel, head list */
  { &_url_mbox_registrar, &_mailbox_mbox_registrar, 0, &registrar[2] },
  { &_url_unix_registrar, &_mailbox_unix_registrar, 0, &registrar[3] },
  { &_url_maildir_registrar, &_mailbox_maildir_registrar, 0, &registrar[4] },
  { &_url_mmdf_registrar, &_mailbox_mmdf_registrar, 0, &registrar[5] },
  { &_url_pop_registrar, &_mailbox_pop_registrar, 0, &registrar[6] },
  { &_url_imap_registrar, &_mailbox_imap_registrar, 0, &registrar[0] },
};

static void
free_ureg (struct url_registrar *ureg)
{
  if (ureg)
    {
      free (ureg->scheme);
      free (ureg);
    }
}

static void
free_mreg (struct mailbox_registrar *mreg)
{
  if (mreg)
    {
      free (mreg->name);
      free (mreg);
    }
}

int
registrar_add (struct url_registrar *new_ureg,
	       struct mailbox_registrar *new_mreg, int *id)
{
  struct _registrar *entry;
  struct url_registrar *ureg = NULL;
  struct mailbox_registrar *mreg;

  /* Must registrar a mailbox */
  if (new_mreg == NULL)
    return EINVAL;

  /* Mailbox */
  mreg = calloc (1, sizeof (*mreg));
  if (mreg == NULL)
    return ENOMEM;

  if (new_mreg->name)
    {
      mreg->name = strdup (new_mreg->name);
      if (mreg->name == NULL)
	{
	  free (mreg);
	  return ENOMEM;
	}
    }
  mreg->_init = new_mreg->_init;
  mreg->_destroy = new_mreg->_destroy;

  /* URL */
  if (new_ureg)
    {
      ureg = calloc (1, sizeof (*ureg));
      if (ureg == NULL)
	{
	  free_mreg (mreg);
	  return ENOMEM;
	}
      if (new_ureg->scheme)
	{
	  ureg->scheme = strdup (new_ureg->scheme);
	  if (ureg->scheme == NULL)
	    {
	      free_mreg (mreg);
	      free_ureg (ureg);
	      return ENOMEM;
	    }
	}
      ureg->_init = new_ureg->_init;
      ureg->_destroy = new_ureg->_destroy;
    }

  /* Register them to the list */
  entry = calloc (1, sizeof (*entry));
  if (entry == NULL)
    {
      free_mreg (mreg);
      free_ureg (ureg);
      return ENOMEM;
    }
  entry->ureg = ureg;
  entry->mreg = mreg;
  entry->is_allocated = 1;
  entry->next = registrar->next;
  registrar->next = entry;
  if (id)
    *id = (int)entry;
  return 0;
}

int
registrar_remove (int id)
{
  struct _registrar *current, *previous;
  for (previous = registrar, current = registrar->next;
       current != registrar;
       previous = current, current = current->next)
    {
      if ((int)current == id)
        {
          previous->next = current->next;
          if (current->is_allocated)
	    {
	      free_ureg (current->ureg);
	      free_mreg (current->mreg);
	    }
	  free (current);
          return 0;;
        }
    }
  return EINVAL;
}

int
registrar_get (int id,
	      struct url_registrar **ureg, struct mailbox_registrar **mreg)
{
  struct _registrar *current;
  for (current = registrar->next; current != registrar;
       current = current->next)
    {
      if ((int)current == id)
        {
	  if (mreg)
	    *mreg = current->mreg;
	  if (ureg)
	    *ureg = current->ureg;
          return 0;
        }
    }
  return EINVAL;
}

int
registrar_list (struct url_registrar **ureg, struct mailbox_registrar **mreg,
		int *id, registrar_t *reg)
{
  struct _registrar *current;

  if (reg == NULL)
    return EINVAL;

  current = *reg;

  if (current == NULL)
    current = registrar;

  if (current->next == registrar)
    return -1;

  if (ureg)
    *ureg = current->ureg;

  if (mreg)
    *mreg = current->mreg;

  if (id)
    *id = (int)current;

  *reg = current->next;

  return 0;
}

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

#include <string.h>
#include <strings.h>

#include <stdlib.h>
#include <stdio.h>
#include <mailutils/sys/pop3.h>
#include <mailutils/sys/iterator.h>

static int  p_add_ref              __P ((iterator_t));
static int  p_release              __P ((iterator_t));
static int  p_destroy              __P ((iterator_t));
static int  p_first                __P ((iterator_t));
static int  p_next                 __P ((iterator_t));
static int  p_current              __P ((iterator_t, void *));
static int  p_is_done              __P ((iterator_t));

static struct _iterator_vtable p_i_vtable =
{
  /* Base.  */
  p_add_ref,
  p_release,
  p_destroy,

  p_first,
  p_next,
  p_current,
  p_is_done
};

int
pop3_iterator_create (pop3_t pop3, iterator_t *piterator)
{
  struct p_iterator *p_iterator;

  p_iterator = malloc (sizeof *p_iterator);
  if (p_iterator == NULL)
    return MU_ERROR_NO_MEMORY;

  p_iterator->base.vtable = &p_i_vtable;
  p_iterator->ref = 1;
  p_iterator->item = NULL;
  p_iterator->done = 0;
  p_iterator->pop3= pop3;
  *piterator = &p_iterator->base;
  return 0;
}

static int
p_add_ref (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  return ++p_iterator->ref;
}

static int
p_release (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (--p_iterator->ref == 0)
    {
      p_destroy (iterator);
      return 0;
    }
  return p_iterator->ref;
}

static int
p_destroy (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (!p_iterator->done)
    {
      char buf[128];
      size_t n = 0;
      while (pop3_readline (p_iterator->pop3, buf, sizeof buf, &n) > 0
	     && n > 0)
	n = 0;
    }
  if (p_iterator->item)
    free (p_iterator->item);
  p_iterator->pop3->state = POP3_NO_STATE;
  free (iterator);
  return 0;
}


static int
p_first  (iterator_t iterator)
{
  return p_next (iterator);
}

static int
p_next (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  size_t n = 0;
  int status;

  if (p_iterator->done)
    return 0;

  status = pop3_readline (p_iterator->pop3, NULL, 0, &n);
  if (status != 0)
    return status;

  if (n == 0)
    {
      p_iterator->done = 1;
      p_iterator->pop3->state = POP3_NO_STATE;
      return 0;
    }

  if (p_iterator->item)
    free (p_iterator->item);

  switch (p_iterator->pop3->state)
    {
    case POP3_CAPA_RX:
      {
	char *buf;

	buf = calloc (n + 1, 1);
	if (buf == NULL)
	  return MU_ERROR_NO_MEMORY;

	/* Consume.  */
	pop3_readline (p_iterator->pop3, buf, n + 1, NULL);
	if (n && buf[n - 1] == '\n')
	  buf[n - 1] = '\0';
	p_iterator->item = buf;
      }
      break;

    case POP3_UIDL_RX:
      {
	unsigned msgno;
	char *space;
	char *buf = calloc (n + 1, 1);

	if (buf == NULL)
	  return MU_ERROR_NO_MEMORY;

	p_iterator->item = calloc (1, sizeof (struct list_item));
	if (p_iterator->item == NULL)
	  return MU_ERROR_NO_MEMORY;

	/* Consume.  */
	pop3_readline (p_iterator->pop3, buf, n + 1, NULL);
	msgno = 0;
	space = strchr (buf, ' ');
	if (space)
	  {
	    *space++ = '\0';
	    msgno = strtoul (buf, NULL, 10);
	  }
	if (space && space[strlen (space) - 1] == '\n')
	  space[strlen (space) - 1] = '\0';
	if (space == NULL)
	  space = (char *)"";
	((struct uidl_item *)(p_iterator->item))->msgno = msgno;
	((struct uidl_item *)(p_iterator->item))->uidl = strdup (space);
	free (buf);
      }
      break;

    case POP3_LIST_RX:
      {
	char *buf = calloc (n + 1, 1);
	unsigned msgno;
	size_t size;

	if (buf == NULL)
	  return MU_ERROR_NO_MEMORY;

	p_iterator->item = calloc (1, sizeof (struct list_item));
	if (p_iterator->item == NULL)
	  return MU_ERROR_NO_MEMORY;

	/* Consume.  */
	pop3_readline (p_iterator->pop3, buf, n + 1, NULL);
	size = msgno = 0;
	sscanf (buf, "%d %d", &msgno, &size);
	((struct list_item *)(p_iterator->item))->msgno = msgno;
	((struct list_item *)(p_iterator->item))->size = size;
	free (buf);
      }
      break;

    default:
    }

  return 0;
}

static int
p_is_done (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  return p_iterator->done;
}

static int
p_current (iterator_t iterator, void *item)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (item)
    {
      switch (p_iterator->pop3->state)
	{
	case POP3_CAPA_RX:
	case POP3_UIDL_RX:
	  *((char **)item) = p_iterator->item;
	  break;

	case POP3_LIST_RX:
	  *((struct list_item **)item) = p_iterator->item;
	  break;

	default:
	}
    }
  p_iterator->item = NULL;
  return 0;
}

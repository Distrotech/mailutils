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
  monitor_create (&p_iterator->lock);
  *piterator = &p_iterator->base;
  return 0;
}

static int
p_add_ref (iterator_t iterator)
{
  int status = 0;
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (p_iterator)
    {
      monitor_lock (p_iterator->lock);
      status = ++p_iterator->ref;
      monitor_unlock (p_iterator->lock);
    }
  return status;
}

static int
p_release (iterator_t iterator)
{
  int status = 0;
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (p_iterator)
    {
      monitor_lock (p_iterator->lock);
      status = --p_iterator->ref;
      if (status <= 0)
	{
	  monitor_unlock (p_iterator->lock);
	  p_destroy (iterator);
	  return 0;
	}
      monitor_unlock (p_iterator->lock);
    }
  return status;
}

static int
p_destroy (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (p_iterator)
    {
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
      monitor_destroy (p_iterator->lock);
      free (p_iterator);
    }
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
  int status = 0;

  if (p_iterator)
    {
      monitor_lock (p_iterator->lock);
      if (!p_iterator->done)
	{
	  /* The first readline will not consume the buffer, we just need to
	     know how much to read.  */
	  status = pop3_readline (p_iterator->pop3, NULL, 0, &n);
	  if (status == 0)
	    {
	      if (n)
		{
		  char *buf;
		  buf = calloc (n + 1, 1);
		  if (buf)
		    {
		      /* Consume.  */
		      pop3_readline (p_iterator->pop3, buf, n + 1, NULL);
		      if (buf[n - 1] == '\n')
			buf[n - 1] = '\0';
		      if (p_iterator->item)
			free (p_iterator->item);
		      p_iterator->item = buf;
		    }
		  else
		    status = MU_ERROR_NO_MEMORY;
		}
	      else
		{
		  p_iterator->done = 1;
		  p_iterator->pop3->state = POP3_NO_STATE;
		}
	    }
	}
      monitor_unlock (p_iterator->lock);
    }
  return status;
}

static int
p_is_done (iterator_t iterator)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  int status = 1;
  if (p_iterator)
    {
      monitor_lock (p_iterator->lock);
      status = p_iterator->done;
      monitor_unlock (p_iterator->lock);
    }
  return status;
}

static int
p_current (iterator_t iterator, void *item)
{
  struct p_iterator *p_iterator = (struct p_iterator *)iterator;
  if (p_iterator)
    {
      monitor_lock (p_iterator->lock);
      if (item)
	{
	  *((char **)item) = p_iterator->item;
	  p_iterator->item = NULL;
	}
      monitor_unlock (p_iterator->lock);
    }
  return 0;
}

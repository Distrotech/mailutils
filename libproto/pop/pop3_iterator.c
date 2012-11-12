/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2004-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <strings.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <mailutils/pop3.h>
#include <mailutils/sys/pop3.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>

static int  pop3_itr_dup     (void **ptr, void *owner);
static int  pop3_itr_destroy (mu_iterator_t itr, void *owner);
static int  pop3_itr_first   (void *owner);
static int  pop3_itr_next    (void *woner);
static int  pop3_itr_getitem (void *owner, void **pret, const void **pkey);
static int  pop3_itr_delitem (void *owner, void *data);
static int  pop3_itr_finished_p (void *owner);

struct pop3_iterator
{
  mu_pop3_t pop3;
  mu_stream_t stream;
  int done;
  char *item;
  char *rdbuf;
  size_t rdsize;
};

int
mu_pop3_iterator_create (mu_pop3_t pop3, mu_iterator_t *piterator)
{
  struct pop3_iterator *pop3_iterator;
  mu_iterator_t iterator;
  int status;

  pop3_iterator = malloc (sizeof *pop3_iterator);
  if (pop3_iterator == NULL)
    return ENOMEM;

  status = mu_pop3_stream_create (pop3, &pop3_iterator->stream);
  if (status)
    {
      free (pop3_iterator);
      return status;
    }
  pop3_iterator->item = NULL;
  pop3_iterator->rdbuf = NULL;
  pop3_iterator->rdsize = 0;
  pop3_iterator->done = 0;
  pop3_iterator->pop3 = pop3;

  status = mu_iterator_create (&iterator, pop3_iterator);
  if (status != 0)
    {
      free (pop3_iterator);
      return status;
    }
  mu_iterator_set_first (iterator, pop3_itr_first);
  mu_iterator_set_next (iterator, pop3_itr_next);
  mu_iterator_set_getitem (iterator, pop3_itr_getitem);
  mu_iterator_set_finished_p (iterator, pop3_itr_finished_p);
  mu_iterator_set_delitem (iterator, pop3_itr_delitem);
  mu_iterator_set_destroy (iterator, pop3_itr_destroy);
  mu_iterator_set_dup (iterator, pop3_itr_dup);

  *piterator = iterator;
  return 0;
}

static int
pop3_itr_dup (void **ptr, void *owner)
{
  struct pop3_iterator *pop3_iterator = (struct pop3_iterator *)owner;
  struct pop3_iterator *clone = malloc (sizeof *pop3_iterator);
  if (clone == NULL)
    return ENOMEM;
  /* let the assignement operator copy the elements.  */
  *clone = *pop3_iterator;
  *ptr = clone;
  return 0;
}

static int
pop3_itr_destroy (mu_iterator_t iterator, void *owner)
{
  struct pop3_iterator *pop3_iterator = (struct pop3_iterator *)owner;
  /* Delicate situation if they did not finish to drain the result
     We take the approach to do it for the user.  FIXME: Not sure
     if this is the rigth thing to do. The other way is to close the stream  */
  if (!pop3_iterator->done)
    {
      char buf[128];
      size_t n = 0;
      mu_stream_t str = pop3_iterator->pop3->carrier;
      while (mu_stream_readline (str, buf, sizeof buf, &n) > 0
	     && n > 0)
	n = 0;
    }
  if (pop3_iterator->item)
    free (pop3_iterator->item);
  if (pop3_iterator->rdbuf)
    free (pop3_iterator->rdbuf);
  pop3_iterator->pop3->state = MU_POP3_NO_STATE;
  free (pop3_iterator);
  return 0;
}

static int
pop3_itr_first  (void *data)
{
  return pop3_itr_next (data);
}

static int
pop3_itr_next (void *owner)
{
  struct pop3_iterator *pop3_iterator = (struct pop3_iterator *)owner;
  int status = 0;
  size_t n;
  
  status = mu_stream_getline (pop3_iterator->stream, &pop3_iterator->rdbuf,
			      &pop3_iterator->rdsize, &n);
  
  if (status || n == 0)
    {
      pop3_iterator->done = 1;
      pop3_iterator->pop3->state = MU_POP3_NO_STATE;
      return 0;
    }

  n = mu_rtrim_class (pop3_iterator->rdbuf, MU_CTYPE_SPACE);
  if (n == 1 && pop3_iterator->rdbuf[0] == '.')
    {
      pop3_iterator->done = 1;
      pop3_iterator->pop3->state = MU_POP3_NO_STATE;
    }
  else
    pop3_iterator->item = pop3_iterator->rdbuf;
  return status;
}

static int
pop3_itr_getitem (void *owner, void **item, const void **pkey)
{
  struct pop3_iterator *pop3_iterator = (struct pop3_iterator *)owner;
  if (item)
    {
      *((char **)item) = pop3_iterator->item;
      pop3_iterator->item = NULL;
    }
  if (pkey)
    *pkey = NULL;
  return 0;
}

static int
pop3_itr_finished_p (void *owner)
{
  struct pop3_iterator *pop3_iterator = (struct pop3_iterator *)owner;
  return pop3_iterator->done;
}

static int
pop3_itr_delitem (void *owner, void *item)
{
  struct pop3_iterator *pop3_iterator = (struct pop3_iterator *)owner;
  return *((char **)item) == pop3_iterator->item ? 
    MU_ITR_DELITEM_NEXT : MU_ITR_DELITEM_NOTHING;
}


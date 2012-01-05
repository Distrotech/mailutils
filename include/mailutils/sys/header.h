/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

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

#ifndef _HEADER0_H
#define _HEADER0_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/header.h>
#include <mailutils/assoc.h>
#include <mailutils/iterator.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The structure members are offset that point to the begin/end of header
   fields.  */
struct mu_hdrent
{
  struct mu_hdrent *prev;
  struct mu_hdrent *next;
  size_t fn;
  size_t nlen;
  size_t fv;
  size_t vlen;
  size_t nlines;
};

struct _mu_header
{
  /* Data.  */
  char *spool;
  size_t spool_size;
  size_t spool_used;
  struct mu_hdrent *head, *tail;
  int flags;

  size_t numhdr;
  size_t numlines;
  size_t size;
  
  /* Temporary storage */
  mu_stream_t mstream;
  
  /* Stream.  */
  mu_stream_t stream;

  /* Iterators */
  mu_iterator_t itr;

  /* Methods */
  int (*_fill) (void *data, char **, size_t *);
  void *data;
};

#ifdef __cplusplus
}
#endif

#endif /* _HEADER0_H */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2008-2012, 2014-2016 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_OPOOL_H
#define _MAILUTILS_OPOOL_H

#include <mailutils/types.h>
#include <setjmp.h>

#ifndef MU_OPOOL_BUCKET_SIZE
# define MU_OPOOL_BUCKET_SIZE 1024
#endif

/* Flags for mu_opool_create call: */
#define MU_OPOOL_DEFAULT 0
#define MU_OPOOL_ENOMEMABRT 0x01 /* Abort on ENOMEM error */

/* Create an object pool.  */
int mu_opool_create (mu_opool_t *pret, int flags);
int mu_opool_set_bucket_size (mu_opool_t opool, size_t size);
int mu_opool_get_bucket_size (mu_opool_t opool, size_t *psize);

struct mu_nonlocal_jmp
{
  jmp_buf buf;
  struct mu_nonlocal_jmp *next;
};

typedef struct mu_nonlocal_jmp mu_nonlocal_jmp_t;

void mu_opool_setjmp (mu_opool_t opool, mu_nonlocal_jmp_t *err);
void mu_opool_clrjmp (mu_opool_t opool);

#define mu_opool_setup_nonlocal_jump(p,jb)	\
  do						\
    {						\
      int __rc = setjmp (jb.buf);		\
      if (__rc)					\
	return __rc;				\
      mu_opool_setjmp (p, &jb);			\
    }						\
 while (0)
   
/* Merge all data from *SRC into *DST.  If the latter is NULL, create
   it.  On success, free *SRC and initialize it with NULL. */
int mu_opool_union (mu_opool_t *dst, mu_opool_t *src);

/* Clear all data from the pool, so next mu_opool_append* call will
   begin a new object. */
void mu_opool_clear (mu_opool_t opool);

/* Free object OBJ from the pool.  If OBJ is NULL, free all created objects,
   including the one being built */
void mu_opool_free (mu_opool_t pool, void *obj);

/* Destroy the pool, reclaim any memory associated with it. */
void mu_opool_destroy (mu_opool_t *popool);

/* Allocate SIZE bytes in the pool. */
int mu_opool_alloc (mu_opool_t opool, size_t size);

/* Append to the current object N bytes pointed to by STR. */
int mu_opool_append (mu_opool_t opool, const void *str, size_t n);

/* Append to the current object a nul-terminated string STR. */
int mu_opool_appendz (mu_opool_t opool, const char *str);

/* Append a single byte C to the current object. */
int mu_opool_append_char (mu_opool_t opool, char c);

/* Return size of the current object in the pool. */
size_t mu_opool_size (mu_opool_t opool);

/* Coalesce all data collected so far (by using mu_opool_append* calls)
   into a single contiguous memory chunk.  Return the size of the resulting
   object in *PSIZE, unless PSIZE==NULL.  Return 0 on success, error code on
   failure (unless the pool was created with `memerr' option, see
   mu_opool_create, above). */
int mu_opool_coalesce (mu_opool_t opool, size_t *psize);

/* Copy at most SIZE bytes of collected data into BUF.  Return the
   actual number of bytes copied. */
size_t mu_opool_copy (mu_opool_t opool, void *buf, size_t size);

/* Return the pointer to the current object head chunk.  If mu_opool_coalesce 
   was called before, the returned value points to the entire object.
   If PSIZE is not NULL, store the size of the head chunk to *PSIZE. */     
void *mu_opool_head (mu_opool_t opool, size_t *psize);

/* Finishes the object being constructed.  Returns pointer to the object,
   and its size in PSIZE (unless it is NULL).

   Equivalent to:
   mu_opool_coalesce (opool, NULL);
   p = mu_opool_head (opool, psize);
   mu_opool_clear (opool);
   return p; */
void *mu_opool_finish (mu_opool_t opool, size_t *psize);


/* Similar to mu_opool_finish, but also detaches the created object from the
   pool, so that the latter can be destroyed without affecting the object.
   The returned pointer should be deallocated with free(3) when no longer
   needed. */
void *mu_opool_detach (mu_opool_t opool, size_t *psize);

/* Append SIZE bytes from DATA to the pool and return the pointer to the
   created object. */
void *mu_opool_dup (mu_opool_t pool, void const *data, size_t size);

int mu_opool_get_iterator (mu_opool_t opool, mu_iterator_t *piterator);

#endif

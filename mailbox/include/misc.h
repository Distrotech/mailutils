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

#ifndef _MISC_H
#define _MISC_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# if __STDC__
#  define __P(x)  x
# else
#  define __P(x)
# endif
#endif
extern size_t _cpystr __P ((char *dst, const char *src, size_t size));
extern int parseaddr  __P ((const char *addr, char *buf, size_t bufsz));

#ifdef HAVE_PTHREAD_H
#  define __USE_UNIX98
#  include <pthread.h>
#endif

#ifdef WITH_PTHREAD
#  if 0
#    define RWLOCK_INIT(rwl, attr)  pthread_mutex_init (rwl, attr)
#    define RWLOCK_DESTROY(rwl)     pthread_mutex_destroy (rwl)
#    define RWLOCK_RDLOCK(rwl)      pthread_mutex_lock (rwl)
#    define RWLOCK_TRYRDLOCK(rwl)   pthread_mutex_trylock (rwl)
#    define RWLOCK_WRLOCK(rwl)      pthread_mutex_lock (rwl)
#    define RWLOCK_TRYWRLOCK(rwl)   pthread_mutex_trylock (rwl)
#    define RWLOCK_UNLOCK(rwl)      pthread_mutex_unlock (rwl)
#  else
#    define RWLOCK_INIT(rwl, attr)  pthread_rwlock_init (rwl, attr)
#    define RWLOCK_DESTROY(rwl)     pthread_rwlock_destroy (rwl)
#    define RWLOCK_RDLOCK(rwl)      pthread_rwlock_rdlock (rwl)
#    define RWLOCK_TRYRDLOCK(rwl)   pthread_rwlock_tryrdlock (rwl)
#    define RWLOCK_WRLOCK(rwl)      pthread_rwlock_wrlock (rwl)
#    define RWLOCK_TRYWRLOCK(rwl)   pthread_rwlock_trywrlock (rwl)
#    define RWLOCK_UNLOCK(rwl)      pthread_rwlock_unlock (rwl)
#  endif
#else
#  define RWLOCK_INIT(rwl, attr)    0
#  define RWLOCK_DESTROY(rwl)
#  define RWLOCK_RDLOCK(rwl)
#  define RWLOCK_TRYRDLOCK(rwl)
#  define RWLOCK_WRLOCK(rwl)
#  define RWLOCK_TRYWRLOCK(rwl)
#  define RWLOCK_UNLOCK(rwl)
#  define flockfile(arg)
#  define funlockfile(arg)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MISC_H */

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

#ifndef _MAILUTILS_MONITOR_H
#define _MAILUTILS_MONITOR_H

#include <unistd.h>
#ifdef _POSIX_THREADS
# include <pthread.h>
#endif

#ifdef _POSIX_THREADS

# define monitor_t pthread_mutex_t

# define MU_MONITOR_INITIALIZER  PTHREAD_MUTEX_INITIALIZER

# define monitor_create(m)  pthread_mutex_init (m, NULL)
# define monitor_destroy(m) pthread_mutex_destroy (m)

# define monitor_cleanup_push(routine, arg) pthread_cleanup_push (routine, arg)
# define monitor_cleanup_pop(execute)       pthread_cleanup_pop (execute)

# define monitor_lock(m)   pthread_mutex_lock (&m)
# define monitor_unlock(m) pthread_mutex_unlock (&m)

#else

# define monitor_t int

# define MU_MONITOR_INITIALIZER 0

# define monitor_create(m) (*m = 0)
# define monitor_destroy(m) (*m = 0)

# define monitor_cleanup_push(routine, arg) {
# define monitor_cleanup_pop(execute)       }

# define monitor_lock(m) (m = 1)
# define monitor_unlock(m)  (m = 0)

#endif

#endif /* _MAILUTILS_MONITOR_H */

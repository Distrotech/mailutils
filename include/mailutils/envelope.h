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

#ifndef _MAILUTILS_ENVELOPE_H
# define _MAILUTILS_ENVELOPE_H

#include <sys/types.h>
#include <mailutils/mu_features.h>

#ifdef __cplusplus
extern "C" { /*}*/
#endif

struct _envelope;
typedef struct _envelope *envelope_t;

int envelope_create (envelope_t *, void *);
void envelope_destroy (envelope_t *, void *);
void * envelope_get_owner (envelope_t);
int envelope_set_sender (envelope_t, int (*_sender) __P ((envelope_t, char *, size_t, size_t*)), void *);
int envelope_sender (envelope_t, char *, size_t, size_t *);
int envelope_set_date (envelope_t, int (*_date) __P ((envelope_t, char *, size_t , size_t *)), void *);
int envelope_date (envelope_t, char *, size_t, size_t *);

#endif /* _MAILUTILS_ENVELOPE_H */

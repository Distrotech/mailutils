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

#ifndef _MAILUTILS_BODY_H
#define _MAILUTILS_BODY_H

#include <sys/types.h>
#include <mailutils/stream.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

#ifdef _cplusplus
extern "C" {
#endif

/* forward declaration */
struct _body;
typedef struct _body *body_t;

extern int body_create       __P ((body_t *, void *owner));
extern void body_destroy     __P ((body_t *, void *owner));
extern void * body_get_owner __P ((body_t));

extern int body_get_stream   __P ((body_t, stream_t *));
extern int body_set_stream   __P ((body_t, stream_t, void *owner));

extern int body_get_filename __P ((body_t, char *, size_t, size_t *));

extern int body_size         __P ((body_t, size_t*));
extern int body_set_size     __P ((body_t, int (*_size)
				   __P ((body_t, size_t*)), void *owner));
extern int body_lines        __P ((body_t, size_t *));
extern int body_set_lines    __P ((body_t, int (*_lines)
				   __P ((body_t, size_t*)), void *owner));

#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_BODY_H */

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
#include <mailutils/mu_features.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _body;
typedef struct _body *body_t;

extern int  body_ref            __P ((body_t));
extern void body_release        __P ((body_t *));
extern int  body_destroy        __P ((body_t));

extern int  body_is_modified    __P ((body_t));
extern int  body_clear_modified __P ((body_t));

extern int  body_get_stream     __P ((body_t, stream_t *));

extern int  body_get_property   __P ((body_t, property_t *));

extern int  body_get_size       __P ((body_t, size_t*));
extern int  body_get_lines      __P ((body_t, size_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_BODY_H */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_PROPERTY_H
#define _MAILUTILS_PROPERTY_H

#include <sys/types.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  property_create   __P ((property_t *, void *));
extern void property_destroy __P ((property_t *, void *));
extern void *property_get_owner __P ((property_t));

extern int  property_set_value __P ((property_t, const char *,
				     const char *, int));
extern int property_get_value __P ((property_t, const char *, char *,
				    size_t, size_t *));

/* Helper functions.  */
extern int property_set  __P ((property_t, const char *));
extern int property_unset __P ((property_t, const char *));
extern int property_is_set __P ((property_t, const char *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_PROPERTY_H */

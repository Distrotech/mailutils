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

#ifndef _MAILUTILS_ATTRIBUTE_H
#define _MAILUTILS_ATTRIBUTE_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_ATTRIBUTE_ANSWERED 0x01
#define MU_ATTRIBUTE_FLAGGED  0x02
#define MU_ATTRIBUTE_DELETED  0x04
#define MU_ATTRIBUTE_DRAFT    0x08
#define MU_ATTRIBUTE_SEEN     0x10
#define MU_ATTRIBUTE_READ     0x20
#define MU_ATTRIBUTE_MODIFIED 0x40

/* A message is recent if the current session is the first session
   to have been notified about it. Practically, a message is considered
   "recent" if it does not have MU_ATTRIBUTE_SEEN set. For consistency
   a pseudo-attribute is provided: */
#define MU_ATTRIBUTE_RECENT   0 

#define MU_ATTRIBUTE_IS_UNSEEN(f) \
      ((f) == 0 || ! ((f) & MU_ATTRIBUTE_SEEN))

#define MU_ATTRIBUTE_IS_UNREAD(f) \
      ((f) == 0 || ! ((f) & MU_ATTRIBUTE_READ))

extern int attribute_create          __P ((attribute_t *, void *));
extern void attribute_destroy        __P ((attribute_t *, void *));
extern void * attribute_get_owner    __P ((attribute_t));
extern int attribute_is_modified     __P ((attribute_t));
extern int attribute_clear_modified  __P ((attribute_t));
extern int attribute_set_modified    __P ((attribute_t attr));

extern int attribute_is_userflag     __P ((attribute_t, int));
extern int attribute_is_seen         __P ((attribute_t));
extern int attribute_is_answered     __P ((attribute_t));
extern int attribute_is_flagged      __P ((attribute_t));
extern int attribute_is_deleted      __P ((attribute_t));
extern int attribute_is_draft        __P ((attribute_t));
extern int attribute_is_recent       __P ((attribute_t));
extern int attribute_is_read         __P ((attribute_t));

extern int attribute_set_userflag    __P ((attribute_t, int));
extern int attribute_set_seen        __P ((attribute_t));
extern int attribute_set_answered    __P ((attribute_t));
extern int attribute_set_flagged     __P ((attribute_t));
extern int attribute_set_deleted     __P ((attribute_t));
extern int attribute_set_draft       __P ((attribute_t));
extern int attribute_set_recent      __P ((attribute_t));
extern int attribute_set_read        __P ((attribute_t));

extern int attribute_unset_userflag  __P ((attribute_t, int));
extern int attribute_unset_seen      __P ((attribute_t));
extern int attribute_unset_answered  __P ((attribute_t));
extern int attribute_unset_flagged   __P ((attribute_t));
extern int attribute_unset_deleted   __P ((attribute_t));
extern int attribute_unset_draft     __P ((attribute_t));
extern int attribute_unset_recent    __P ((attribute_t));
extern int attribute_unset_read      __P ((attribute_t));

extern int attribute_get_flags       __P ((attribute_t, int *));
extern int attribute_set_flags       __P ((attribute_t, int));
extern int attribute_unset_flags     __P ((attribute_t, int));

extern int attribute_set_set_flags   __P ((attribute_t, int (*_set_flags)
					   __PMT ((attribute_t, int)), void *));
extern int attribute_set_unset_flags __P ((attribute_t, int (*_unset_flags)
					   __PMT ((attribute_t, int)), void *));
extern int attribute_set_get_flags   __P ((attribute_t, int (*_get_flags)
					   __PMT ((attribute_t, int *)), void *));
extern int attribute_is_equal        __P ((attribute_t, attribute_t att2));

extern int attribute_copy            __P ((attribute_t, attribute_t));

extern int attribute_to_string       __P ((attribute_t, char *, size_t,
					   size_t *));
extern int string_to_flags           __P ((const char *, int *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ATTRIBUTE_H */

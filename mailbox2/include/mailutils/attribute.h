/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_ATTRIBUTE_H
#define _MAILUTILS_ATTRIBUTE_H

#include <mailutils/mu_features.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_ATTRIBUTE_ANSWERED 0x000001
#define MU_ATTRIBUTE_FLAGGED  0x000002
#define MU_ATTRIBUTE_DELETED  0x000004
#define MU_ATTRIBUTE_DRAFT    0x000008
#define MU_ATTRIBUTE_SEEN     0x000010
#define MU_ATTRIBUTE_READ     0x000020
#define MU_ATTRIBUTE_MODIFIED 0x100000
#define MU_ATTRIBUTE_RECENT   0x000000

extern int  attribute_ref             __P ((attribute_t));
extern void attribute_destroy         __P ((attribute_t *));

extern int  attribute_is_seen         __P ((attribute_t));
extern int  attribute_is_answered     __P ((attribute_t));
extern int  attribute_is_flagged      __P ((attribute_t));
extern int  attribute_is_deleted      __P ((attribute_t));
extern int  attribute_is_draft        __P ((attribute_t));
extern int  attribute_is_recent       __P ((attribute_t));
extern int  attribute_is_read         __P ((attribute_t));
extern int  attribute_is_modified     __P ((attribute_t));

extern int  attribute_set_seen        __P ((attribute_t));
extern int  attribute_set_answered    __P ((attribute_t));
extern int  attribute_set_flagged     __P ((attribute_t));
extern int  attribute_set_deleted     __P ((attribute_t));
extern int  attribute_set_draft       __P ((attribute_t));
extern int  attribute_set_recent      __P ((attribute_t));
extern int  attribute_set_read        __P ((attribute_t));
extern int  attribute_set_modified    __P ((attribute_t));

extern int  attribute_unset_seen      __P ((attribute_t));
extern int  attribute_unset_answered  __P ((attribute_t));
extern int  attribute_unset_flagged   __P ((attribute_t));
extern int  attribute_unset_deleted   __P ((attribute_t));
extern int  attribute_unset_draft     __P ((attribute_t));
extern int  attribute_unset_recent    __P ((attribute_t));
extern int  attribute_unset_read      __P ((attribute_t));
extern int  attribute_unset_modified  __P ((attribute_t));

extern int  attribute_is_userflags    __P ((attribute_t, int));
extern int  attribute_set_userflags   __P ((attribute_t, int));
extern int  attribute_unset_userflags __P ((attribute_t, int));
extern int  attribute_get_userflags   __P ((attribute_t, int *));
extern int  attribute_clear_userflags __P ((attribute_t));


extern int  attribute_get_flags       __P ((attribute_t, int *));
extern int  attribute_set_flags       __P ((attribute_t, int));
extern int  attribute_unset_flags     __P ((attribute_t, int));
extern int  attribute_clear_flags     __P ((attribute_t));

extern int  attribute_is_equal        __P ((attribute_t, attribute_t));
extern int  attribute_copy            __P ((attribute_t dst, attribute_t src));

extern int  attribute_default_create  __P ((attribute_t *));
extern int  attribute_status_create   __P ((attribute_t *, const char *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ATTRIBUTE_H */

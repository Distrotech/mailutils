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

#ifndef _MAILUTILS_ATTRIBUTE_H
#define _MAILUTILS_ATTRIBUTE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _attribute;
typedef struct _attribute * attribute_t;

#define MU_ATTRIBUTE_ANSWERED 0x00001
#define MU_ATTRIBUTE_FLAGGED  0x00002
#define MU_ATTRIBUTE_DELETED  0x00004
#define MU_ATTRIBUTE_DRAFT    0x00008
#define MU_ATTRIBUTE_SEEN     0x00010
#define MU_ATTRIBUTE_READ     0x00020
#define MU_ATTRIBUTE_MODIFIED 0x10000
#define MU_ATTRIBUTE_RECENT   0x00000

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

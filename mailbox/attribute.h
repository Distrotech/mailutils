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

#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

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

extern int attribute_init __P ((attribute_t *, void *owner));
extern void attribute_destroy __P ((attribute_t *, void *owner));

extern int attribute_is_seen __P ((attribute_t));
extern int attribute_is_answered __P ((attribute_t));
extern int attribute_is_flagged __P ((attribute_t));
extern int attribute_is_deleted __P ((attribute_t));
extern int attribute_is_draft __P ((attribute_t));
extern int attribute_is_recent __P ((attribute_t));
extern int attribute_is_read __P ((attribute_t));

extern int attribute_set_seen __P ((attribute_t));
extern int attribute_set_answered __P ((attribute_t));
extern int attribute_set_flagged __P ((attribute_t));
extern int attribute_set_deleted __P ((attribute_t));
extern int attribute_set_draft __P ((attribute_t));
extern int attribute_set_recent __P ((attribute_t));
extern int attribute_set_read __P ((attribute_t));

extern int attribute_unset_seen __P ((attribute_t));
extern int attribute_unset_answered __P ((attribute_t));
extern int attribute_unset_flagged __P ((attribute_t));
extern int attribute_unset_deleted __P ((attribute_t));
extern int attribute_unset_draft __P ((attribute_t));
extern int attribute_unset_recent __P ((attribute_t));
extern int attribute_unset_read __P ((attribute_t));

extern int attribute_is_equal __P ((attribute_t att1, attribute_t att2));

#ifdef __cplusplus
}
#endif

#endif /* _ATTRIBUTE_H */

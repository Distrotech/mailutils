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

#ifndef _ATTRIBUTE0_H
#define _ATTRIBUTE0_H

#include <attribute.h>

#include <sys/types.h>

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

#define MU_ATTRIBUTE_SEEN ((int)1)
#define MU_ATTRIBUTE_ANSWERED (MU_ATTRIBUTE_SEEN << 1)
#define MU_ATTRIBUTE_FLAGGED  (MU_ATTRIBUTE_ANSWERED << 1)
#define MU_ATTRIBUTE_DELETED  (MU_ATTRIBUTE_FLAGGED << 1)
#define MU_ATTRIBUTE_DRAFT    (MU_ATTRIBUTE_DELETED << 1)
#define MU_ATTRIBUTE_RECENT   (MU_ATTRIBUTE_DRAFT << 1)
#define MU_ATTRIBUTE_READ     (MU_ATTRIBUTE_RECENT << 1)

struct _attribute
{
  size_t flag;
  void *message;
};

#ifdef __cplusplus
}
#endif

#endif /* _ATTRIBUTE0_H */

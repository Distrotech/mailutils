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

#ifndef _MAILUTILS_ENVELOPE_H
# define _MAILUTILS_ENVELOPE_H

#include <mailutils/address.h>
#include <mailutils/mutil.h>

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

struct _envelope;
typedef struct _envelope *envelope_t;

extern int envelope_add_ref   __P ((envelope_t));
extern int envelope_release   __P ((envelope_t));
extern int envelope_destroy   __P ((envelope_t));

extern int envelope_sender    __P ((envelope_t, address_t *));
extern int envelope_recipient __P ((envelope_t, address_t *));
extern int envelope_date      __P ((envelope_t, struct tm *, struct mu_timezone *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ENVELOPE_H */

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

#ifndef _MAILUTILS_ENVELOPE_H
# define _MAILUTILS_ENVELOPE_H

#include <mailutils/mu_features.h>
#include <mailutils/types.h>
#include <mailutils/address.h>
#include <mailutils/mutil.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  envelope_ref       __P ((envelope_t));
extern void envelope_destroy   __P ((envelope_t *));

extern int  envelope_get_sender    __P ((envelope_t, address_t *));
extern int  envelope_set_sender    __P ((envelope_t, address_t));
extern int  envelope_get_recipient __P ((envelope_t, address_t *));
extern int  envelope_set_recipient __P ((envelope_t, address_t));
extern int  envelope_get_date      __P ((envelope_t, struct tm *, struct mu_timezone *));
extern int  envelope_set_date      __P ((envelope_t, struct tm *, struct mu_timezone *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ENVELOPE_H */

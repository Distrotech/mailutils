/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILER_H
#define _MAILER_H

#include "message.h"
#include <sys/types.h>

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
struct _mailer;
typedef struct _mailer *mailer_t;

extern int mailer_create			__P ((mailer_t *, message_t));
extern int mailer_connect		__P ((mailer_t, char *host));
extern int mailer_disconnect	__P ((mailer_t));
extern int mailer_send_header	__P ((mailer_t, message_t));
extern int mailer_send_message	__P ((mailer_t, message_t));

#ifdef _cplusplus
}
#endif

#endif /* _MAILER_H */

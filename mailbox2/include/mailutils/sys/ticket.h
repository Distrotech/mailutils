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

#ifndef _MAILUTILS_SYS_TICKET_H
#define _MAILUTILS_SYS_TICKET_H

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <mailutils/ticket.h>

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

struct _ticket_vtable
{
  int (*add_ref) __P ((ticket_t));
  int (*release) __P ((ticket_t));
  int (*destroy) __P ((ticket_t));

  int (*pop)     __P ((ticket_t, const char *, char **));
};

struct _ticket
{
  struct _ticket_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_TICKET_H */

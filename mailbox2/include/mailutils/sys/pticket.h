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

#ifndef _MAILUTILS_SYS_PTICKET_H
#define _MAILUTILS_SYS_PTICKET_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/sys/ticket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _ticket_prompt
{
  struct _ticket base;
  mu_refcount_t refcount;
};

extern int  _ticket_prompt_ctor __P ((struct _ticket_prompt *));
extern void _ticket_prompt_dtor __P ((ticket_t));
extern int  _ticket_prompt_ref  __P ((ticket_t));
extern void _ticket_prompt_destroy __P ((ticket_t *));
extern int  _ticket_prompt_pop     __P ((ticket_t, const char *, char **));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_PTICKET_H */

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

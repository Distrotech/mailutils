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

#ifndef _AUTH0_H
#define _AUTH0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/auth.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _ticket
{
  void *owner;
  char *challenge;
  void *data;
  int  (*_pop)      __P ((ticket_t, url_t, const char *challenge, char **));
  void (*_destroy)  __P ((ticket_t));
};

struct _authority
{
  void *owner;
  ticket_t ticket;
  int (*_authenticate) __P ((authority_t));
};

struct _wicket
{
  char *filename;
  int (*_get_ticket) __P ((wicket_t, const char *, const char *, ticket_t *));
};


#ifdef __cplusplus
}
#endif

#endif /* _AUTH0_H */

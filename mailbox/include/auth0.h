/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _AUTH0_H
#define _AUTH0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/auth.h>
#include <mailutils/list.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _ticket
{
  void *owner;
  char *challenge;
  void *data;
  int  (*_pop)      (ticket_t, url_t, const char *challenge, char **);
  void (*_destroy)  (ticket_t);
};

struct _authority
{
  void *owner;
  ticket_t ticket;
  list_t auth_methods; /* list of int (*_authenticate) (authority_t)s; */
};

struct _wicket
{
  char *filename;
  int (*_get_ticket) (wicket_t, const char *, const char *, ticket_t *);
};


#ifdef __cplusplus
}
#endif

#endif /* _AUTH0_H */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005  Free Software Foundation, Inc.

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

#ifndef _URL0_H
#define _URL0_H	1

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/url.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _url
{
  /* Data */
  char  *name;
  char  *scheme;
  char  *user;
  char  *passwd;
  char  *auth;
  char  *host;
  long  port;
  char  *path;
  char  *query;


  void  *data;

  void  (*_destroy)    (url_t url);

  /* Methods */
  int   (*_get_scheme) (const url_t, char *, size_t, size_t *);
  int   (*_get_user)   (const url_t, char *, size_t, size_t *);
  int   (*_get_passwd) (const url_t, char *, size_t, size_t *);
  int   (*_get_auth)   (const url_t, char *, size_t, size_t *);
  int   (*_get_host)   (const url_t, char *, size_t, size_t *);
  int   (*_get_port)   (const url_t, long *);
  int   (*_get_path)   (const url_t, char *, size_t, size_t *);
  int   (*_get_query)  (const url_t, char *, size_t, size_t *);
};


#ifdef __cplusplus
}
#endif

#endif /* URL_H */

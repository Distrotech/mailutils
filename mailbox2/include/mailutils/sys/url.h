/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_SYS_URL_H
#define _MAILUTILS_SYS_URL_H	1

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

  void  (*_destroy)    __P ((url_t url));

  /* Methods */
  int   (*_get_scheme) __P ((const url_t, char *, size_t, size_t *));
  int   (*_get_user)   __P ((const url_t, char *, size_t, size_t *));
  int   (*_get_passwd) __P ((const url_t, char *, size_t, size_t *));
  int   (*_get_auth)   __P ((const url_t, char *, size_t, size_t *));
  int   (*_get_host)   __P ((const url_t, char *, size_t, size_t *));
  int   (*_get_port)   __P ((const url_t, long *));
  int   (*_get_path)   __P ((const url_t, char *, size_t, size_t *));
  int   (*_get_query)  __P ((const url_t, char *, size_t, size_t *));
};


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_URL_H */

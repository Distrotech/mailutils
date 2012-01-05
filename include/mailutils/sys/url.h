/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_URL_H
# define _MAILUTILS_SYS_URL_H

# include <mailutils/url.h>

# ifdef __cplusplus
extern "C" {
# endif

struct _mu_url
{
  int flags;  /* See MU_URL_ flags in ../url.h */
  /* Data */
  char  *name;
  char  *scheme;
  char  *user;
  mu_secret_t secret;
  char  *auth;
  char  *host;
  short port;
  char  *portstr;
  char  *path;
  char  **fvpairs;
  int   fvcount;

  char  **qargv;
  int   qargc;
  
  void  *data;

  void  (*_destroy)    (mu_url_t url);

  /* Methods */
  int   (*_get_scheme) (const mu_url_t, char *, size_t, size_t *);
  int   (*_get_user)   (const mu_url_t, char *, size_t, size_t *);
  int   (*_get_secret) (const mu_url_t, mu_secret_t *);
  int   (*_get_auth)   (const mu_url_t, char *, size_t, size_t *);
  int   (*_get_host)   (const mu_url_t, char *, size_t, size_t *);
  int   (*_get_port)   (const mu_url_t, unsigned *);
  int   (*_get_portstr)(const mu_url_t, char *, size_t, size_t *);
  int   (*_get_path)   (const mu_url_t, char *, size_t, size_t *);
  int   (*_get_query)  (const mu_url_t, char *, size_t, size_t *);
  int   (*_uplevel)    (const mu_url_t, mu_url_t *);
};

# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_URL_H */

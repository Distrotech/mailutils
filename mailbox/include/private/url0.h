/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _URL0_H
#define _URL0_H	1

#include <url.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# if __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*!__P */

struct _url
{
  /* Data */
  char  *scheme;
  char  *user;
  char  *passwd; /* encoded ?? */
  char  *host;
  long  port;
  char  *path;
  char  *query;
  int   id;


  void  *data;

  int   (*_create)        __P ((url_t *url, const char *name));
  void  (*_destroy)     __P ((url_t *url));

  /* Methods */
  int   (*_get_id)      __P ((const url_t, int *id));

  int   (*_get_scheme)  __P ((const url_t, char *scheme,
			      size_t len, size_t *n));
  int   (*_get_mscheme) __P ((const url_t, char **, size_t *n));

  int   (*_get_user)    __P ((const url_t, char *user,
			      size_t len, size_t *n));
  int   (*_get_muser)   __P ((const url_t, char **, size_t *n));

  int   (*_get_passwd)  __P ((const url_t, char *passwd,
			      size_t len, size_t *n));
  int   (*_get_mpasswd) __P ((const url_t, char **, size_t *n));

  int   (*_get_host)    __P ((const url_t, char *host,
			      size_t len, size_t *n));
  int   (*_get_mhost)   __P ((const url_t, char **, size_t *n));

  int   (*_get_port)    __P ((const url_t, long *port));

  int   (*_get_path)    __P ((const url_t, char *path,
			      size_t len, size_t *n));
  int   (*_get_mpath)   __P ((const url_t, char **, size_t *n));

  int   (*_get_query)   __P ((const url_t, char *query,
			      size_t len, size_t *n));
  int   (*_get_mquery)  __P ((const url_t, char **, size_t *n));
};


/* IMAP */

/* Mailto */

/* UNIX MBOX */

/* Maildir */

/* MMDF */

/* POP3 */
struct _url_pop;
typedef struct _url_pop * url_pop_t;

struct _url_pop
{
  /* we use the fields from url_t */
  /* user, passwd, host, port */
  char * auth;
  int (*_get_auth) __P ((const url_pop_t, char *, size_t, size_t *));
};

#define MU_POP_PORT 110

/* pop*/
extern int url_pop_get_auth (const url_t, char *, size_t, size_t *);

/* UNIX MBOX */

#ifdef __cplusplus
}
#endif

#endif /* URL_H */

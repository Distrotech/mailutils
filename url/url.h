/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#ifndef _URL_H
#define _URL_H	1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*!__P */

/* forward declaration */
struct _url;
typedef struct _url * url_t;

struct url_type
{
  char *scheme;
  int  len;
  char *description;
  int  id;
  int  (*_init)     __P ((url_t *, const char * name));
  void (*_destroy)  __P ((url_t *));
};

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

  void  *data;
  const struct url_type * utype;

  /* Methods */
  int   (*_get_scheme) __P ((const url_t, char *scheme, int len));
  int   (*_get_user)   __P ((const url_t, char *user, int len));
  int   (*_get_passwd) __P ((const url_t, char *passwd, int len));
  int   (*_get_host)   __P ((const url_t, char *host, int len));
  int   (*_get_port)   __P ((const url_t, long *port));
  int   (*_get_path)   __P ((const url_t, char *path, int len));
  int   (*_get_query)  __P ((const url_t, char *query, int len));
  int   (*_get_id)     __P ((const url_t, int  *id));
};

extern int  url_init    __P ((url_t *, const char *name));
extern void url_destroy __P ((url_t *));

extern int url_list_type   __P ((struct url_type list[], int len));
extern int url_list_mtype  __P ((struct url_type *mlist[], int *len));
extern int url_add_type    __P ((struct url_type *utype));
extern int url_remove_type __P ((const struct url_type *utype));

#ifdef __GNUC__
# define INLINE __inline__
#else
# define INLINE
#endif

extern INLINE int url_get_scheme  __P ((const url_t, char *scheme, int len));
extern INLINE int url_get_user    __P ((const url_t, char *user, int len));
extern INLINE int url_get_passwd  __P ((const url_t, char *passwd, int len));
extern INLINE int url_get_host    __P ((const url_t, char *host, int len));
extern INLINE int url_get_port    __P ((const url_t, long *port));
extern INLINE int url_get_path    __P ((const url_t, char *path, int len));
extern INLINE int url_get_query   __P ((const url_t, char *query, int len));
extern INLINE int url_get_id      __P ((const url_t, int  *id));

#ifdef URL_MACROS
# define url_get_scheme(url, scheme, n) url->_get_scheme (url, scheme, n)
# define url_get_user(url, user, n)     url->_get_user (url, user, n)
# define url_get_passwd(url, passwd, n) url->_get_passwd (url, passwd, n)
# define url_get_host(url, host, n)     url->_get_host (url, host, n)
# define url_get_port(url, port)        url->_get_port (url, port)
# define url_get_path(url, path, n)     url->_get_path (url, path, n)
# define url_get_query(url, query, n)   url->_get_query (url, query, n)
# define url_get_id(url, id)            url->_get_id (url, id)
#endif /* URL_MACROS */

#ifdef __cplusplus
}
#endif

#endif /* URL_H */

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

#ifndef _URL_H
#define _URL_H	1

#include <sys/types.h>
#include <stdlib.h>

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

/* forward declaration */
struct _url;
typedef struct _url * url_t;

struct url_type
{
  char *scheme;
  size_t  len;
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

  int   (*_get_id)      __P ((const url_t, int  *id));
};

extern int  url_init    __P ((url_t *, const char *name));
extern void url_destroy __P ((url_t *));

extern int url_list_type   __P ((struct url_type list[], size_t l, size_t *n));
extern int url_list_mtype  __P ((struct url_type **mlist, size_t *n));
extern int url_add_type    __P ((struct url_type *utype));
extern int url_remove_type __P ((const struct url_type *utype));

#ifdef __GNUC__
# define INLINE __inline__
#else
# define INLINE
#endif

extern INLINE int url_get_scheme  __P ((const url_t, char *sch,
					size_t, size_t *));
extern INLINE int url_get_mscheme __P ((const url_t, char **, size_t *));
extern INLINE int url_get_user    __P ((const url_t, char *usr,
					size_t, size_t *));
extern INLINE int url_get_muser   __P ((const url_t, char **, size_t *));
extern INLINE int url_get_passwd  __P ((const url_t, char *passwd,
					size_t, size_t *));
extern INLINE int url_get_mpasswd __P ((const url_t, char **, size_t *));
extern INLINE int url_get_host    __P ((const url_t, char *host,
					size_t, size_t *));
extern INLINE int url_get_mhost   __P ((const url_t, char **, size_t *));
extern INLINE int url_get_port    __P ((const url_t, long *port));
extern INLINE int url_get_path    __P ((const url_t, char *path,
					size_t, size_t *));
extern INLINE int url_get_mpath   __P ((const url_t, char **, size_t *));
extern INLINE int url_get_query   __P ((const url_t, char *qeury,
					size_t, size_t *));
extern INLINE int url_get_mquery  __P ((const url_t, char **, size_t *));
extern INLINE int url_get_id      __P ((const url_t, int *id));

#ifdef URL_MACROS
# define url_get_scheme(url, scheme, l, n) url->_get_scheme (url, scheme,l, n)
# define url_get_mscheme(url, mscheme, n)  url->_get_mscheme (url, mscheme, n)
# define url_get_user(url, user, l, n)     url->_get_user (url, user, l, n)
# define url_get_muser(url, muser, n)      url->_get_muser (url, muser, n)
# define url_get_passwd(url, passwd, l, n) url->_get_passwd (url, passwd, l, n)
# define url_get_mpasswd(url, mpasswd, n)  url->_get_mpasswd (url, mpasswd, n)
# define url_get_host(url, host, l, n)     url->_get_host (url, host, l, n)
# define url_get_mhost(url, mhost, n)      url->_get_mhost (url, mhost, n)
# define url_get_port(url, port)           url->_get_port (url, port)
# define url_get_path(url, path, l, n)     url->_get_path (url, path, l, n)
# define url_get_mpath(url, mpath, n)      url->_get_mpath (url, mpath, n)
# define url_get_query(url, query, l, n)   url->_get_query (url, query, l, n)
# define url_get_mquery(url, mquery, n)    url->_get_mquery (url, mquery, n)
# define url_get_id(url, id)               url->_get_id (url, id)
#endif /* URL_MACROS */

#ifdef __cplusplus
}
#endif

#endif /* URL_H */

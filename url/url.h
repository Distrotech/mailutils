/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _URL_H
#define _URL_H	1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*!__P */

/* forward declaration */
struct _url;
typedef struct _url * url_t;

struct _url
{
    /* Data */
#define URL_POP ((int)1)
#define URL_IMAP (URL_POP << 1)
#define URL_MBOX (URL_IMAP << 1)
    int type;
    char *scheme;
    char *user;
    char *passwd; /* encoded ?? */
    char *host;
    long port;
    char *path;
    char *query;

    
    void * data;
    int  (*_create)      __P ((url_t *, const char *));
    void (*_destroy)     __P ((url_t *));

    /* Methods */
    int  (*_get_scheme)  __P ((const url_t, char *, unsigned int));
    int  (*_get_user)    __P ((const url_t, char *, unsigned int));
    int  (*_get_passwd)  __P ((const url_t, char *, unsigned int));
    int  (*_get_host)    __P ((const url_t, char *, unsigned int));
    int  (*_get_port)    __P ((const url_t, long *));
    int  (*_get_path)    __P ((const url_t, char *, unsigned int));
    int  (*_get_query)   __P ((const url_t, char *, unsigned int));
    int  (*_is_pop)      __P ((const url_t));
    int  (*_is_imap)     __P ((const url_t));
    int  (*_is_unixmbox) __P ((const url_t));
};

extern int url_mailto_create __P ((url_t *, const char *name));
extern void url_mailto_destroy __P ((url_t *));

/* foward decl */
struct _url_pop;
typedef struct _url_pop * url_pop_t;

struct _url_pop
{
    /* we use the fields from url_t */
    /* user, passwd, host, port */
    char * auth;
    int (*_get_auth) __P ((const url_pop_t, char *, unsigned int));
};

extern int url_pop_create __P ((url_t *, const char *name));
extern void url_pop_destroy __P ((url_t *));

extern int url_imap_create __P ((url_t *, const char *name));
extern void url_imap_destroy __P ((url_t *));

extern int url_mbox_create __P ((url_t *, const char *name));
extern void url_mbox_destroy __P ((url_t *));

extern int url_create __P ((url_t *, const char *name));
extern void url_destroy __P ((url_t *));

#ifdef __GNUC__
#define INLINE __inline__
#else
#define INLINE
#endif

extern INLINE int url_get_scheme  (const url_t, char *, unsigned int);
extern INLINE int url_get_user    (const url_t , char *, unsigned int);
extern INLINE int url_get_passwd  (const url_t, char *, unsigned int);
extern INLINE int url_get_host    (const url_t, char *, unsigned int);
extern INLINE int url_get_port    (const url_t, long *);
extern INLINE int url_get_path    (const url_t, char *, unsigned int);
extern INLINE int url_get_query   (const url_t, char *, unsigned int);
extern INLINE int url_is_pop      (const url_t);
extern INLINE int url_is_imap     (const url_t);
extern INLINE int url_is_unixmbox (const url_t);
/* pop*/
extern INLINE int url_pop_get_auth (const url_t, char *, unsigned int);

#ifdef URL_MACROS
# define url_get_scheme(url, prot, n)		url->_get_scheme(url, prot, n)
# define url_get_user(url, user, n)		url->_get_user(url, user, n)
# define url_get_passwd(url, passwd, n)		url->_get_passwd(url, passwd, n)
# define url_get_host(url, host, n)		url->_get_host(url, host, n)
# define url_get_port(url, port)		url->_get_port(url, port)
# define url_get_path(url, path, n)		url->_get_path(url, path, n)
# define url_get_query(url, query, n)		url->_get_query(url, query, n)

/* what we support so far */
# define url_is_pop(url)			(url->type & URL_POP)
# define url_is_imap(url)			(url->type & URL_IMAP)
# define url_is_unixmbox(url)			(url->type & URL_MBOX)

/* pop */
# define url_pop_get_auth(url, auth, n)		(((url_pop_t) \
	    (url->data))->_get_auth(url->data, auth, n))
#endif /* URL_MACROS */

#ifdef __cplusplus
}
#endif
#endif /* URL_H */

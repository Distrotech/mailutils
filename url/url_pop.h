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

#ifndef _URL_POP_H
#define _URL_POP_H	1

#include <url.h>

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

extern int  url_pop_init    __P ((url_t *, const char *name));
extern void url_pop_destroy __P ((url_t *));

#define MU_POP_PORT 110

extern struct url_type _url_pop_type;

#ifndef INLINE
# ifdef __GNUC__
#  define INLINE __inline__
# else
#  define INLINE
# endif
#endif

/* pop*/
extern int url_pop_get_auth (const url_t, char *, size_t, size_t *);

#ifdef MU_URL_MACROS
/* pop */
# define url_pop_get_auth(url, auth, l, n)	(((url_pop_t) \
	    (url->data))->_get_auth(url->data, auth, l, n))
#endif /* MU_URL_MACROS */

#ifdef __cplusplus
}
#endif

#endif /* URL_POP_H */

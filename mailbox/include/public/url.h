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

extern int  url_init    __P ((url_t *, const char *name));
extern void url_destroy __P ((url_t *));

extern int url_get_id __P ((const url_t, int *id));

extern int url_get_scheme  __P ((const url_t, char *sch,
					size_t, size_t *));
extern int url_get_mscheme __P ((const url_t, char **, size_t *));

extern int url_get_user    __P ((const url_t, char *usr,
					size_t, size_t *));
extern int url_get_muser   __P ((const url_t, char **, size_t *));

extern int url_get_passwd  __P ((const url_t, char *passwd,
					size_t, size_t *));
extern int url_get_mpasswd __P ((const url_t, char **, size_t *));

extern int url_get_host    __P ((const url_t, char *host,
					size_t, size_t *));
extern int url_get_mhost   __P ((const url_t, char **, size_t *));

extern int url_get_port    __P ((const url_t, long *port));

extern int url_get_path    __P ((const url_t, char *path,
					size_t, size_t *));
extern int url_get_mpath   __P ((const url_t, char **, size_t *));

extern int url_get_query   __P ((const url_t, char *qeury,
					size_t, size_t *));
extern int url_get_mquery  __P ((const url_t, char **, size_t *));

#ifdef __cplusplus
}
#endif

#endif /* URL_H */

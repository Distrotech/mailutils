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

#ifndef _MAILUTILS_URL_H
#define _MAILUTILS_URL_H	1

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  url_create    (url_t *, const char *name);
extern void url_destroy   (url_t *);
extern int  url_parse     (url_t);

extern int url_get_scheme  (const url_t, char *, size_t, size_t *);
extern int url_get_user    (const url_t, char *, size_t, size_t *);
extern int url_get_passwd  (const url_t, char *, size_t, size_t *);
extern int url_get_auth    (const url_t, char *, size_t, size_t *);
extern int url_get_host    (const url_t, char *, size_t, size_t *);
extern int url_get_port    (const url_t, long *);
extern int url_get_path    (const url_t, char *, size_t, size_t *);
extern int url_get_query   (const url_t, char *, size_t, size_t *);
extern const char* url_to_string   (const url_t);

extern int url_is_scheme   (url_t, const char* scheme);

extern int url_is_same_scheme (url_t, url_t);
extern int url_is_same_user   (url_t, url_t);
extern int url_is_same_path   (url_t, url_t);
extern int url_is_same_host   (url_t, url_t);
extern int url_is_same_port   (url_t, url_t);

extern char* url_decode    (const char *s);

extern int url_is_ticket   (url_t ticket, url_t url);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_URL_H */

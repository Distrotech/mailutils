/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <url.h>

int
(url_get_scheme) (const url_t url, char *scheme, int n)
{ return url->_get_scheme(url, scheme, n); }

int
(url_get_user) (const url_t url, char *user, int n)
{ return url->_get_user(url, user, n); }

int
(url_get_passwd) (const url_t url, char *passwd, int n)
{ return url->_get_passwd(url, passwd, n); }

int
(url_get_host) (const url_t url, char *host, int n)
{ return url->_get_host(url, host, n); }

int
(url_get_port) (const url_t url, long *port)
{ return url->_get_port(url, port); }

int
(url_get_path) (const url_t url, char *path, int n)
{ return url->_get_path(url, path, n); }

int
(url_get_query) (const url_t url, char *query, int n)
{ return url->_get_query(url, query, n); }

int
(url_get_id) (const url_t url, int *id)
{ return url->_get_id (url, id); }

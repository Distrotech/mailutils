/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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
#include <errno.h>

int (url_get_scheme) (const url_t url, char *scheme, size_t len, size_t *n)
{
  return (url) ? url->_get_scheme(url, scheme, len, n) : EINVAL;
}

int (url_get_mscheme) (const url_t url, char **mscheme, size_t *n)
{
  return (url) ? url->_get_mscheme(url, mscheme, n) : EINVAL;
}

int (url_get_user) (const url_t url, char *user, size_t len, size_t *n)
{
  return (url) ? url->_get_user(url, user, len, n) : EINVAL;
}

int (url_get_muser) (const url_t url, char **muser, size_t *n)
{
  return (url) ? url->_get_muser(url, muser, n) : EINVAL;
}

int (url_get_passwd) (const url_t url, char *passwd, size_t len, size_t *n)
{
  return (url) ? url->_get_passwd(url, passwd, len, n) : EINVAL;
}

int (url_get_mpasswd) (const url_t url, char **mpasswd, size_t *n)
{
  return (url) ? url->_get_mpasswd(url, mpasswd, n) : EINVAL;
}

int (url_get_host) (const url_t url, char *host, size_t len, size_t *n)
{
  return (url) ? url->_get_host(url, host, len, n) : EINVAL;
}

int (url_get_mhost) (const url_t url, char **mhost, size_t *n)
{
  return (url) ? url->_get_mhost(url, mhost, n) : EINVAL;
}

int (url_get_port) (const url_t url, long *port)
{
  return (url) ? url->_get_port(url, port) : EINVAL;
}

int (url_get_path) (const url_t url, char *path, size_t len, size_t *n)
{
  return (url) ? url->_get_path(url, path, len, n) : EINVAL;
}

int (url_get_mpath) (const url_t url, char **mpath, size_t *n)
{
  return (url) ? url->_get_mpath(url, mpath, n) : EINVAL;
}

int (url_get_query) (const url_t url, char *query, size_t len, size_t *n)
{
  return (url) ? url->_get_query(url, query, len, n) : EINVAL;
}

int (url_get_mquery) (const url_t url, char **mquery, size_t *n)
{
  return (url) ? url->_get_mquery(url, mquery, n) : EINVAL;
}

int (url_get_id) (const url_t url, int *id)
{
  return (url) ? url->_get_id (url, id) : EINVAL;
}

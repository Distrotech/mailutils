/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <mailutils/cpp/url.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Url
//

Url :: Url (const std::string& str)
{
  int status = url_create (&url, str.c_str ());
  if (status)
    throw Exception ("Url::Url", status);
}

Url :: Url (const char* str)
{
  int status = url_create (&url, str);
  if (status)
    throw Exception ("Url::Url", status);
}

Url :: Url (const url_t url)
{
  if (url == 0)
    throw Exception ("Url::Url", EINVAL);

  this->url = url;
}

Url :: ~Url ()
{
  url_destroy (&url);
}

void
Url :: Parse ()
{
  int status = url_parse (url);
  if (status)
    throw Exception ("Url::Parse", status);
}

long
Url :: GetPort ()
{
  long port;
  int status = url_get_port (url, &port);
  if (status)
    throw Exception ("Url::GetPort", status);
  return port;
}

std::string
Url :: GetScheme ()
{
  int status = url_get_scheme (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetScheme", status);
  return std::string (buf);
}

std::string
Url :: GetUser ()
{
  int status = url_get_user (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetUser", status);
  return std::string (buf);
}

std::string
Url :: GetPasswd ()
{
  int status = url_get_passwd (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetPasswd", status);
  return std::string (buf);
}

std::string
Url :: GetAuth ()
{
  int status = url_get_auth (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetAuth", status);
  return std::string (buf);
}
 
std::string
Url :: GetHost ()
{
  int status = url_get_host (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetHost", status);
  return std::string (buf);
}

std::string
Url :: GetPath ()
{
  int status = url_get_path (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetPath", status);
  return std::string (buf);
}

std::string
Url :: GetQuery ()
{
  int status = url_get_query (url, buf, sizeof (buf), NULL);
  if (status)
    throw Exception ("Url::GetQuery", status);
  return std::string (buf);
}


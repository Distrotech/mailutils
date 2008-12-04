/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006, 2007, 2008 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA
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
  int status = mu_url_create (&url, str.c_str ());
  if (status)
    throw Exception ("Url::Url", status);
}

Url :: Url (const char* str)
{
  int status = mu_url_create (&url, str);
  if (status)
    throw Exception ("Url::Url", status);
}

Url :: Url (const mu_url_t url)
{
  if (url == 0)
    throw Exception ("Url::Url", EINVAL);

  this->url = url;
}

Url :: ~Url ()
{
  mu_url_destroy (&url);
}

void
Url :: parse ()
{
  int status = mu_url_parse (url);
  if (status)
    throw Exception ("Url::parse", status);
}

long
Url :: getPort ()
{
  long port;
  int status = mu_url_get_port (url, &port);
  if (status)
    throw Exception ("Url::getPort", status);
  return port;
}

std::string
Url :: getScheme ()
{
  int status = mu_url_get_scheme (url, buf, sizeof (buf), NULL);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::getScheme", status);
  return std::string (buf);
}

std::string
Url :: getUser ()
{
  int status = mu_url_get_user (url, buf, sizeof (buf), NULL);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::getUser", status);
  return std::string (buf);
}

std::string
Url :: getPasswd ()
{
  int status = mu_url_get_passwd (url, buf, sizeof (buf), NULL);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::getPasswd", status);
  return std::string (buf);
}

std::string
Url :: getAuth ()
{
  int status = mu_url_get_auth (url, buf, sizeof (buf), NULL);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::getAuth", status);
  return std::string (buf);
}
 
std::string
Url :: getHost ()
{
  int status = mu_url_get_host (url, buf, sizeof (buf), NULL);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::getHost", status);
  return std::string (buf);
}

std::string
Url :: getPath ()
{
  int status = mu_url_get_path (url, buf, sizeof (buf), NULL);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::getPath", status);
  return std::string (buf);
}

std::vector<std::string>
Url :: getQuery ()
{
  size_t argc;
  char **argv;
  
  int status = mu_url_sget_query (url, &argc, &argv);
  if (status)
    throw Exception ("Url::getQuery", status);

  std::vector<std::string> params;

  for (int i = 0; i < argc; i++)
    params.push_back (argv[i]);

  return params;
}


/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.

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
Url :: get_port ()
{
  long port;
  int status = mu_url_get_port (url, &port);
  if (status)
    throw Exception ("Url::get_port", status);
  return port;
}

std::string
Url :: get_scheme ()
{
  char *buf = NULL;
  int status = mu_url_aget_scheme (url, &buf);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::get_scheme", status);
  return std::string (buf ? buf : "");
}

std::string
Url :: get_user ()
{
  char *buf = NULL;
  int status = mu_url_aget_user (url, &buf);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::get_user", status);
  return std::string (buf ? buf : "");
}

std::string
Url :: get_passwd ()
{
  char *buf = NULL;
  int status = mu_url_aget_passwd (url, &buf);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::get_passwd", status);
  return std::string (buf ? buf : "");
}

std::string
Url :: get_auth ()
{
  char *buf = NULL;
  int status = mu_url_aget_auth (url, &buf);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::get_auth", status);
  return std::string (buf ? buf : "");
}
 
std::string
Url :: get_host ()
{
  char *buf = NULL;
  int status = mu_url_aget_host (url, &buf);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::get_host", status);
  return std::string (buf ? buf : "");
}

std::string
Url :: get_path ()
{
  char *buf = NULL;
  int status = mu_url_aget_path (url, &buf);
  if (status == MU_ERR_NOENT)
    return "";
  else if (status)
    throw Exception ("Url::get_path", status);
  return std::string (buf ? buf : "");
}

std::vector<std::string>
Url :: get_query ()
{
  size_t argc;
  char **argv;
  
  int status = mu_url_sget_query (url, &argc, &argv);
  if (status)
    throw Exception ("Url::get_query", status);

  std::vector<std::string> params;

  for (int i = 0; i < argc; i++)
    params.push_back (argv[i]);

  return params;
}

std::string
Url :: to_string ()
{
  const char *str = mu_url_to_string (url);
  return std::string (str ? str : "");
}

namespace mailutils
{
  std::ostream& operator << (std::ostream& os, Url& url) {
    return os << url.to_string ();
  };
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifndef _MUCPP_FOLDER_H
#define _MUCPP_FOLDER_H

#include <errno.h>
#include <mailutils/folder.h>
#include <mailutils/cpp/error.h>
#include <mailutils/cpp/list.h>
#include <mailutils/cpp/stream.h>
#include <mailutils/cpp/url.h>

namespace mailutils
{

class Folder
{
 protected:
  mu_folder_t folder;

 public:
  Folder (const std::string& name);
  Folder (const mu_folder_t);
  Folder& operator = (const Folder&);
  ~Folder ();

  void open ();
  void open (int flag);
  void close ();

  List& list (const std::string& dirname, void* pattern, size_t max_level);
  List& enumerate (const std::string& name, void* pattern,
		   int flags, size_t max_level,
		   mu_folder_enumerate_fp enumfun, void* enumdata);

  Url& get_url ();
};

}

#endif // not _MUCPP_FOLDER_H


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE
#endif
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <string.h>

#include <mailutils/mu_auth.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/mailbox.h>



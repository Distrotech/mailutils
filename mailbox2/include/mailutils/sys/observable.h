/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_OBSERVABLE_H
#define _MAILUTILS_SYS_OBSERVABLE_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/list.h>
#include <mailutils/observable.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _observable
{
  mu_list_t list;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_OBSERVER_H */

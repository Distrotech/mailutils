/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _PROPERTY0_H
#define _PROPERTY0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/property.h>
#include <mailutils/monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

struct property_item
{
  char *key;
  char *value;
  int set;
  struct property_item *next;
};

struct _property
{
  struct property_item *items;
  void *owner;
  monitor_t lock;
};

#ifdef __cplusplus
}
#endif

#endif /* _PROPERTY0_H */

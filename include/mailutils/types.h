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

#ifndef _MAILUTILS_TYPES_H
#define _MAILUTILS_TYPES_H

#include <mailutils/mu_features.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _locker;
struct _mailbox;
struct _message;
struct _address;

typedef struct _locker  *locker_t;
typedef struct _mailbox *mailbox_t;
typedef struct _message *message_t;
typedef struct _address *address_t;

#ifdef __cplusplus
}
#endif

#endif


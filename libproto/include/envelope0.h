/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _ENVELOPE0_H
#define _ENVELOPE0_H

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <mailutils/envelope.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_envelope
{
  void *owner;
  int (*_destroy) (mu_envelope_t);
  int (*_sender)    (mu_envelope_t, char *, size_t, size_t*);
  int (*_date)    (mu_envelope_t, char *, size_t , size_t *);
};

#endif /* _ENVELOPE0_H */
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

#ifndef _MAILUTILS_FILTER_H
#define _MAILUTILS_FILTER_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type.  */
#define MU_FILTER_DECODE 0
#define MU_FILTER_ENCODE 1

/* Direction.  */
#define MU_FILTER_READ  MU_STREAM_READ
#define MU_FILTER_WRITE MU_STREAM_WRITE
#define MU_FILTER_RDWR  MU_STREAM_RDWR

struct _filter_record
{
  const char *name;
  int  (*_filter)     __P ((filter_t));
  void *data;

  /* Stub function return the fields.  */
  int (*_is_filter)  __P ((filter_record_t, const char *));
  int (*_get_filter) __P ((filter_record_t, int (*(*_filter)) __P ((filter_t))));
};


extern int filter_create   __P ((stream_t *, stream_t, const char*, int, int));
extern int filter_get_list __P ((list_t *));

/* List of defaults.  */
extern filter_record_t rfc822_filter;
extern filter_record_t qp_filter; /* quoted-printable.  */
extern filter_record_t base64_filter;
extern filter_record_t binary_filter;
extern filter_record_t bit8_filter;
extern filter_record_t bit7_filter;

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_FILTER_H */

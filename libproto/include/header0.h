/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2007 Free Software Foundation, Inc.

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

#ifndef _HEADER0_H
#define _HEADER0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/header.h>
#include <mailutils/assoc.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The structure members are offset that point to the begin/end of header
   fields.  */
struct _hdr
{
  char *fn;
  char *fn_end;
  char *fv;
  char *fv_end;
};

/* The blurb member represents the headers, hdr_count the number of distinct
   header field and the layout is done by struct_hdr *hdr.  */
struct _mu_header
{
  /* Owner.  */
  void *owner;

  /* Data.  */
  mu_stream_t mstream;
  size_t stream_len;
  char *blurb;
  size_t blurb_len;
  size_t hdr_count;
  struct _hdr *hdr;
  int flags;

  mu_assoc_t cache;
  
  /* Stream.  */
  mu_stream_t stream;
  int (*_get_value) (mu_header_t, const char *, char *, size_t , size_t *);
  int (*_set_value) (mu_header_t, const char *, const char *, int);
  int (*_lines)     (mu_header_t, size_t *);
  int (*_size)      (mu_header_t, size_t *);
  int (*_fill)      (mu_header_t, char *, size_t, mu_off_t, size_t *);
};

#ifdef __cplusplus
}
#endif

#endif /* _HEADER0_H */

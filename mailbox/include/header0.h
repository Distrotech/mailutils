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

#ifndef _HEADER0_H
#define _HEADER0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/header.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

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
struct _header
{
  /* Owner.  */
  void *owner;

  /* Data.  */
  char *temp_blurb;
  size_t temp_blurb_len;
  char *blurb;
  size_t blurb_len;
  size_t hdr_count;
  struct _hdr *hdr;
  size_t fhdr_count;
  struct _hdr *fhdr;
  int flags;

  /* Streams.  */
  stream_t stream;
  int (*_get_value) __P ((header_t, const char *, char *, size_t , size_t *));
  int (*_get_fvalue) __P ((header_t, const char *, char *, size_t , size_t *));
  int (*_set_value) __P ((header_t, const char *, const char *, int));
  int (*_lines)     __P ((header_t, size_t *));
  int (*_size)      __P ((header_t, size_t *));
  int (*_fill)      __P ((header_t, char *, size_t, off_t, size_t *));
};

#ifdef __cplusplus
}
#endif

#endif /* _HEADER0_H */

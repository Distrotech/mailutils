/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _HEADER0_H
#define _HEADER0_H

#include <sys/types.h>
#include <header.h>
#include <io0.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef _cpluscplus
extern "C" {
#endif

struct _hdr
{
  char *fn;
  char *fn_end;
  char *fv;
  char *fv_end;
};
typedef struct _hdr *hdr_t;

struct _header
{
  size_t num;
  /* Data */
  void *data;
  /* streams */
  istream_t is;
  ostream_t os;

  /* owner ? */
  void *owner;
  int ref_count;

  /* Functions */
  int (*_init)        __P ((header_t *, const char *, size_t, void *owner));
  void (*_destroy)    __P ((header_t *, void *owner));
  int (*_set_value)   __P ((header_t, const char *fn, const char *fv,
			     size_t n, int replace));
  int (*_get_value)   __P ((header_t, const char *fn, char *fv,
			    size_t len, size_t *n));
  int (*_entry_count) __P ((header_t, size_t *));
  int (*_entry_name)  __P ((header_t, size_t num, char *buf,
			    size_t buflen, size_t *nwritten));
  int (*_entry_value) __P ((header_t, size_t num, char *buf,
			    size_t buflen, size_t *nwritten));
  int (*_get_istream) __P ((header_t h, istream_t *is));
  int (*_get_ostream) __P ((header_t h, ostream_t *os));
  int (*_parse)       __P ((header_t, const char *blurb, size_t len));
};

/* rfc822 */
extern int rfc822_init __P ((header_t *ph, const char *blurb,
			     size_t len, void *owner));
extern void rfc822_destroy __P ((header_t *ph, void *owner));

#ifdef _cpluscplus
}
#endif

#endif /* HEADER0_H */

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

/*
 * sieve field cache.
 */

#ifndef SVFIELD_H
#define SVFIELD_H

#define SV_FIELD_CACHE_SIZE 1019

typedef struct sv_field_t
{
  char *name;
  int ncontents;
  char *contents[1];
}
sv_field_t;

typedef struct sv_field_cache_t
{
  sv_field_t *cache[SV_FIELD_CACHE_SIZE];
}
sv_field_cache_t;

extern int sv_field_cache_add (sv_field_cache_t* m, const char *name, char *body);
extern int sv_field_cache_get (sv_field_cache_t* m, const char *name, const char ***body);
extern void sv_field_cache_release (sv_field_cache_t* m);

#endif


/* Replacement for GNU C library function getline

   Copyright (C) 1995, 1997, 1999, 2000, 2001, 2002, 2003 Free
   Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
Licensealong with this program; if not, write to the Free
SoftwareFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301 USA.  */

#ifndef GETLINE_H_
# define GETLINE_H_ 1

# include <stddef.h>
# include <stdio.h>

/* Get ssize_t.  */
# include <sys/types.h>

/* glibc2 has these functions declared in <stdio.h>.  Avoid redeclarations.  */
# if __GLIBC__ < 2

extern ssize_t getline (char **_lineptr, size_t *_linesize, FILE *_stream);

extern ssize_t getdelim (char **_lineptr, size_t *_linesize, int _delimiter,
			  FILE *_stream);

# endif

#endif /* not GETLINE_H_ */

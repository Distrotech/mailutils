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

#include <util.h>
#include <ctype.h>

char *
strupper (char *s)
{
  char *t = s;

   while (*t)
   {
      if (islower ((unsigned)*t))
	*t = toupper ((unsigned)*t);
      t++;
   }
   return s;
}

char *
strlower (char *s)
{
  char *t = s;

   while (*t)
   {
      if (isupper ((unsigned)*t))
	*t = tolower ((unsigned)*t);
      t++;
   }
   return s;
}

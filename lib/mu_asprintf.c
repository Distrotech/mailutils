/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "mu_asprintf.h"
#if !HAVE_DECL_VASPRINTF
extern int vasprintf __P((char **result, const char *format, va_list args));
#endif
#if !HAVE_DECL_ASPRINTF
#if __STDC__
extern int asprintf __P((char **result, const char *format, ...));
#else
extern int asprintf ();
#endif
#endif
     
int
mu_vasprintf (char **result, const char *format, va_list * args)
{
  return vasprintf (result, format, args);
}

extern int
mu_asprintf (char **result, const char *format, ...)
{
  va_list args;
  int done;

  va_start (args, format);
  done = vasprintf (result, format, args);
  va_end (args);

  return done;
}

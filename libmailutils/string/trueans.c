/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <mailutils/cctype.h>
#include <mailutils/nls.h>

int
mu_true_answer_p (const char *p)
{
  if (!p)
    return -1;

  while (*p && mu_isspace (*p))
    p++;

  if (*p)
    {
      /* TRANSLATORS: This is a list of characters which start
	 an affirmative answer. Whenever possible, please preserve
	 'yY' in your translation, e.g., for Euskara:

	 msgstr "yYbB";
      */
      if (strchr (_("yY"), *p))
	return 1;

      /* TRANSLATORS: This is a list of characters which start
	 a negative answer. Whenever possible, please preserve
	 'nN' in your translation, e.g., for Euskara:

	 msgstr "nNeE";
      */
      else if (strchr (_("nN"), *p))
	return 0;
    }
  return -1;
}


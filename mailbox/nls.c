/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/nls.h>

/* Initialize Native Language Support */

void
mu_init_nls (void)
{
#ifdef ENABLE_NLS
  /* Set locale via LC_ALL.  */
#ifdef HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#endif /* HAVE_SETLOCALE */
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif /* ENABLE_NLS */
  return;
}


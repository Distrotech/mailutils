/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2006  Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_NLS_H
#define _MAILUTILS_NLS_H

#include <mailutils/types.h>

/*
   Native Language Support
*/

#ifdef ENABLE_NLS
# ifdef WITH_INCLUDED_LIBINTL
#  include <libgnuintl.h>
# else
#  include <libintl.h>
# endif
# define _(String) gettext(String)
# define N_(String) String
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif /* HAVE_LOCALE_H */
#else
# define _(String) (String)
# define N_(String) String
# define gettext(msgid) (msgid)
# define dgettext(domain, msgid) (msgid)
# define ngettext(sg,pl,cnt) (cnt == 1) ? (sg) : (pl)
# define textdomain(Domain)
# define bindtextdomain(Package, Directory)
#endif /* ENABLE_NLS */

#ifdef __cplusplus
extern "C" {
#endif

extern void mu_init_nls (void);
extern char *mu_set_locale (const char *locale);
void mu_restore_locale (void);

#ifdef __cplusplus
}
#endif

#endif

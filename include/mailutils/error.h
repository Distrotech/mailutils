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

#ifndef _MAILUTILS_ERROR_H
#define _MAILUTILS_ERROR_H

#include <stdarg.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*error_pfn_t) __P ((const char *fmt, va_list ap));

extern int mu_verror __P ((const char *fmt, va_list ap));
extern int mu_error __P ((const char *fmt, ...));
extern void mu_error_set_print __P ((error_pfn_t));

int mu_default_error_printer __P ((const char *fmt, va_list ap));
int mu_syslog_error_printer __P((const char *fmt, va_list ap));
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ERROR_H */

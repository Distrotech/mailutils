/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_DEBUG_H
#define _MAILUTILS_DEBUG_H

#include <stdarg.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_DEBUG_ERROR 0x0001
#define MU_DEBUG_TRACE 0x0002
#define MU_DEBUG_PROT  0x0004

extern int mu_debug_create    (mu_debug_t *, void *owner);
extern void mu_debug_destroy  (mu_debug_t *, void *owner);
extern void * mu_debug_get_owner (mu_debug_t);
extern int mu_debug_set_level (mu_debug_t, size_t level);
extern int mu_debug_get_level (mu_debug_t, size_t *plevel);
extern int mu_debug_print     (mu_debug_t debug, size_t level,
			       const char *format, ...) MU_PRINTFLIKE(3,4);
extern int mu_debug_printv    (mu_debug_t debug, size_t level,
			       const char *format, va_list argp);

typedef int (*mu_debug_printer_fp) (mu_debug_t, size_t level, const char *buf);
  
extern int mu_debug_set_print (mu_debug_t,
			       mu_debug_printer_fp printer,
                               void *owner);
extern mu_debug_printer_fp mu_debug_default_printer;
  
int mu_debug_syslog_printer (mu_debug_t, size_t, const char *);
int mu_debug_stderr_printer (mu_debug_t, size_t, const char *);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_DEBUG_H */

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
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

#ifndef _MAILUTILS_DIAG_H
#define _MAILUTILS_DIAG_H

#include <stdarg.h>

#include <mailutils/types.h>
#include <mailutils/log.h>
#include <mailutils/debug.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *mu_program_name;
extern const char *mu_full_program_name;

#define MU_DIAG_EMERG    MU_LOG_EMERG
#define MU_DIAG_ALERT    MU_LOG_ALERT
#define MU_DIAG_CRIT     MU_LOG_CRIT
#define MU_DIAG_ERROR    MU_LOG_ERROR
#define MU_DIAG_ERR MU_DIAG_ERROR
#define MU_DIAG_WARNING  MU_LOG_WARNING
#define MU_DIAG_NOTICE   MU_LOG_NOTICE
#define MU_DIAG_INFO     MU_LOG_INFO
#define MU_DIAG_DEBUG    MU_LOG_DEBUG
  
void mu_set_program_name (const char *);
void mu_diag_init (void);
void mu_diag_vprintf (int, const char *, va_list);
void mu_diag_cont_vprintf (const char *, va_list);
void mu_diag_printf (int, const char *, ...) MU_PRINTFLIKE(2,3);
void mu_diag_cont_printf (const char *fmt, ...) MU_PRINTFLIKE(1,2);
  
void mu_diag_voutput (int, const char *, va_list);
void mu_diag_output (int, const char *, ...) MU_PRINTFLIKE(2,3);
void mu_diag_at_locus (int level, struct mu_locus const *loc,
		       const char *fmt, ...);

int mu_diag_level_to_syslog (int level);
const char *mu_diag_level_to_string (int level);

void mu_diag_funcall (int level, const char *func,
		      const char *arg, int err);
  
#ifdef __cplusplus
}
#endif

#endif

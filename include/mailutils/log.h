/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_LOG_H
#define _MAILUTILS_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mailutils/types.h>

#define MU_LOG_DEBUG   0
#define MU_LOG_INFO    1
#define MU_LOG_NOTICE  2
#define MU_LOG_WARNING 3
#define MU_LOG_ERROR   4
#define MU_LOG_CRIT    5
#define MU_LOG_ALERT   6
#define MU_LOG_EMERG   7

#define MU_LOGMODE_SEVERITY 0x0001
#define MU_LOGMODE_LOCUS    0x0002

int mu_log_stream_create (mu_stream_t *, mu_stream_t); 
int mu_syslog_stream_create (mu_stream_t *, int);

int mu_severity_from_string (const char *str, unsigned *pn);
int mu_severity_to_string (unsigned n, const char **pstr);
  
extern char *_mu_severity_str[];
extern int _mu_severity_num;
  
#ifdef __cplusplus
}
#endif
  
#endif
  

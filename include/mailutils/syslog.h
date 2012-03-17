/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2008, 2010-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYSLOG_H
#define _MAILUTILS_SYSLOG_H

#include <syslog.h>

#ifdef __cplusplus
extern "C" { 
#endif
  
extern int mu_log_syslog;
extern int mu_log_facility;
extern char *mu_log_tag;
extern int mu_log_print_severity;
extern unsigned mu_log_severity_threshold;
extern int mu_log_session_id;

#define MU_LOG_TAG() (mu_log_tag ? mu_log_tag : mu_program_name)
  
int mu_string_to_syslog_facility (const char *str, int *pfacility);
const char *mu_syslog_facility_to_string (int n);
int mu_string_to_syslog_priority (const char *str, int *pprio);
const char *mu_syslog_priority_to_string (int n);

#ifdef __cplusplus
} 
#endif

#endif

  

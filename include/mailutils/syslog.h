/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYSLOG_H
#define _MAILUTILS_SYSLOG_H

#include <syslog.h>

#ifdef __cplusplus
extern "C" { 
#endif
  
int mu_string_to_syslog_facility (char *str, int *pfacility);
const char *mu_syslog_facility_to_string (int n);
int mu_string_to_syslog_priority (char *str, int *pprio);
const char *mu_syslog_priority_to_string (int n);

#ifdef __cplusplus
} 
#endif

#endif

  
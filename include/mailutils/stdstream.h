/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2011-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_STDSTREAM_H
#define _MAILUTILS_STDSTREAM_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern mu_stream_t mu_strin;
extern mu_stream_t mu_strout;
extern mu_stream_t mu_strerr;

extern const char *mu_program_name;
  
#define MU_STRERR_STDERR  0
#define MU_STRERR_SYSLOG  1
/* #define MU_STRERR_FILE    2 */

#define MU_STDSTREAM_RESET_STRIN  0x01
#define MU_STDSTREAM_RESET_STROUT 0x02
#define MU_STDSTREAM_RESET_STRERR 0x04

#define MU_STDSTREAM_RESET_NONE 0  
#define MU_STDSTREAM_RESET_ALL \
  (MU_STDSTREAM_RESET_STRIN | \
   MU_STDSTREAM_RESET_STROUT | \
   MU_STDSTREAM_RESET_STRERR)
  
void mu_stdstream_setup (int reset_flags);
int mu_stdstream_strerr_create (mu_stream_t *str, int type, int facility,
				int priority, const char *tag,
				const char *fname);
int mu_stdstream_strerr_setup (int type);
  
int mu_printf (const char *fmt, ...) MU_PRINTFLIKE(1,2);
  
#ifdef __cplusplus
}
#endif
  
#endif
  

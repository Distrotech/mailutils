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

#ifndef _MAILUTILS_VERSION_H
#define _MAILUTILS_VERSION_H

#include <mailutils/types.h>  

#ifdef __cplusplus
extern "C" {
#endif

struct mu_conf_option
{
  char *name;
  char *descr;
};
  
extern void mu_print_options (void);
extern void mu_format_options (mu_stream_t, int verbose);
extern void mu_format_conf_option (mu_stream_t,
                                   const struct mu_conf_option *opt,
				   int verbose);
extern const struct mu_conf_option *mu_check_option (char *name);

#ifdef __cplusplus
}
#endif

#endif

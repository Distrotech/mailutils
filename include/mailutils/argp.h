/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_ARGP_H
#define _MAILUTILS_ARGP_H

#include <mailutils/types.h>

#include <argp.h>

#define MODE_INTERACTIVE 0
#define MODE_DAEMON 1

struct daemon_param {
  int mode;
  size_t maxchildren;
  unsigned int port;
  unsigned int timeout;
  int transcript;
};

#ifdef USE_LIBPAM
extern char *pam_service;
#endif
extern int log_facility;
extern int mu_argp_error_code;
extern char *mu_license_text;

#ifdef __cplusplus
extern "C" {
#endif

extern void mu_argp_init __P ((const char *vers, const char *bugaddr));
  
extern void mu_create_argcv __P ((const char *capa[],
				  int argc, char **argv,
				  int *p_argc, char ***p_argv));
extern error_t mu_argp_parse __P ((const struct argp * argp,
				   int *p_argc, char ***p_argv,
				   unsigned flags,
				   const char *capa[],
				   int *arg_index, void *input));
extern int mu_register_capa __P((const char *name, struct argp_child *child));

extern void mu_print_options __P((void));
extern const char *mu_check_option __P((char *name));

#ifdef __cplusplus
}
#endif

#endif


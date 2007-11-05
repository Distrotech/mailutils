/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005,
   2007 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_ARGP_H
#define _MAILUTILS_ARGP_H

#include <mailutils/types.h>
#include <mailutils/cfg.h>
#include <argp.h>
#include <errno.h> /* May declare program_invocation_name */

#define MODE_INTERACTIVE 0
#define MODE_DAEMON 1

struct daemon_param {
  int mode;
  size_t maxchildren;
  unsigned int port;
  unsigned int timeout;
  int transcript;
  char *pidfile;
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

extern void mu_argp_init (const char *vers, const char *bugaddr);
  
extern error_t mu_argp_parse (const struct argp * argp,
			      int *p_argc, char ***p_argv,
			      unsigned flags,
			      const char *capa[],
			      int *arg_index, void *input);
extern int mu_register_capa (const char *name, struct argp_child *child,
			     struct mu_cfg_param *param);

extern void mu_print_options (void);
extern const char *mu_check_option (char *name);

#ifdef __cplusplus
}
#endif

#endif


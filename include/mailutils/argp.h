/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_ARGP_H
#define _MAILUTILS_ARGP_H

#include <mailutils/types.h>

#ifdef HAVE_ARGP_H
# include <argp.h>
#else
# include <mailutils/gnu/argp.h>
#endif

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

#endif


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2007, 2009-2012, 2014-2016 Free Software
   Foundation, Inc.

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

#include <mailutils/nls.h>
#include <mailutils/opt.h>

#define MH_GETOPT_DEFAULT_FOLDER 0x1

void mh_getopt (int *pargc, char ***pargv, struct mu_option *options,
		int flags,
		char *argdoc, char *progdoc, char *extradoc);

void mh_opt_notimpl (struct mu_parseopt *po, struct mu_option *opt,
		     char const *arg);
void mh_opt_notimpl_warning (struct mu_parseopt *po, struct mu_option *opt,
			     char const *arg);
void mh_opt_clear_string (struct mu_parseopt *po, struct mu_option *opt,
			  char const *arg);

void mh_opt_find_file (struct mu_parseopt *po, struct mu_option *opt,
		       char const *arg);
void mh_opt_read_formfile (struct mu_parseopt *po, struct mu_option *opt,
			   char const *arg);
void mh_opt_set_folder (struct mu_parseopt *po, struct mu_option *opt,
			char const *arg);



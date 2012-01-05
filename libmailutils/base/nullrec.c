/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

/* This file provides replacement definitions for mu_record_t defined
   in those MU libraries which are disabled at configure time. */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/registrar.h>

#ifndef ENABLE_IMAP
mu_record_t mu_imap_record = NULL;
mu_record_t mu_imaps_record = NULL;
#endif

#ifndef ENABLE_POP
mu_record_t mu_pop_record = NULL;
mu_record_t mu_pops_record = NULL;
#endif

#ifndef ENABLE_NNTP
mu_record_t mu_nntp_record = NULL;
#endif

#ifndef ENABLE_MH
mu_record_t mu_mh_record = NULL;
#endif

#ifndef ENABLE_MAILDIR
mu_record_t mu_maildir_record = NULL;
#endif

#ifndef WITH_TLS
mu_record_t mu_smtps_record = NULL;
#endif


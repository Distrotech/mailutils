/* This file is part of GNU Mailutils
   Copyright (C) 2007, 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include "mailutils/libcfg.h"

static char *pam_settings;

static struct mu_cfg_param mu_pam_param[] = {
  { "service", mu_cfg_string, &pam_settings, 0, NULL,
    N_("Set PAM service name."),
    N_("name") },
  { NULL }
};

DCL_CFG_CAPA (pam);


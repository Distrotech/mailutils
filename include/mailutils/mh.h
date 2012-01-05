/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

#ifndef _MAILUTILS_MH_H
#define _MAILUTILS_MH_H

extern mu_property_t mu_mh_profile;
extern mu_property_t mu_mh_context;

#define MU_MH_SEQUENCES_FILE ".mh_sequences"
#define MU_MH_USER_PROFILE ".mh_profile"
#define MU_MH_GLOBAL_PROFILE "mh-profile"
#define MU_MH_CONTEXT_FILE "context"

typedef int (*mu_mhprop_iterator_t) (const char *field, const char *value,
				     void *data);

const char *mu_mhprop_get_value (mu_property_t prop, const char *name,
				 const char *defval);
int mu_mhprop_iterate (mu_property_t prop, mu_mhprop_iterator_t fp,
		       void *data);

#endif


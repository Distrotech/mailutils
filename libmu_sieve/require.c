/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012, 2014-2016 Free Software
   Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve-priv.h>

void
mu_sieve_require (mu_sieve_machine_t mach, mu_sieve_slice_t list)
{
  size_t i;
  
  for (i = 0; i < list->count; i++)
    {
      struct mu_sieve_string *str = mu_sieve_string_raw (mach, list, i);
      char *name = str->orig;
      int rc;
      
      if (strcmp (name, "variables") == 0) /* RFC 5229 */
	rc = mu_sieve_require_variables (mach);
      else if (strcmp (name, "relational") == 0) /* RFC 3431 */
	rc = mu_sieve_require_relational (mach, name);
      else if (strcmp (name, "encoded-character") == 0) /* RFC 5228, 2.4.2.4 */
	rc = mu_sieve_require_encoded_character (mach, name);
      else if (strncmp (name, "comparator-", 11) == 0)
	rc = mu_sieve_registry_require (mach, name + 11,
					mu_sieve_record_comparator);
      else if (strncmp (name, "test-", 5)  == 0) /* GNU extension */
	rc = mu_sieve_registry_require (mach, name + 5,
					mu_sieve_record_test);
      else
	rc = mu_sieve_registry_require (mach, name, mu_sieve_record_action);

      if (rc)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, _("can't require %s"),
			    name);
	  mu_i_sv_error (mach);
	}
    }
}
     

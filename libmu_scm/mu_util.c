/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2006, 2007, 2010 Free Software
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
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

SCM_DEFINE (scm_mu_getpwuid, "mu-getpwuid", 1, 0, 0,
            (SCM USER),
"Look up an entry in the user database. USER can be an integer,\n"
"or a string, giving the behaviour of @code{mu_get_auth_by_uid} or\n"
"@code{mu_get_auth_by_name} respectively.\n"
"\n"
"Returns a vector with fields corresponding to those of the @code{mu_auth_data}\n"
"entry in question. If no matching entry was found, returns @code{#f}.\n")
#define FUNC_NAME s_scm_mu_getpwuid
{
  SCM result;
  struct mu_auth_data *entry;
  SCM *ve;
  scm_t_array_handle handle;
  
  result = scm_c_make_vector (8, SCM_UNSPECIFIED);
  ve = scm_vector_writable_elements (result,
				     &handle,
				     NULL, NULL);
  
  if (scm_is_integer (USER))
    {
      entry = mu_get_auth_by_uid (scm_to_int32 (USER));
    }
  else
    {
      SCM_VALIDATE_STRING (1, USER);
      if (scm_is_string (USER))
	USER = scm_from_locale_stringn (scm_i_string_chars (USER), 
					scm_i_string_length (USER));
      entry = mu_get_auth_by_name (scm_i_string_chars (USER));
    }
  if (!entry)
    mu_scm_error (FUNC_NAME, errno,
		  "Cannot get user credentials", SCM_BOOL_F);

  ve[0] = scm_makfrom0str (entry->name);
  ve[1] = scm_makfrom0str (entry->passwd);
  ve[2] = scm_ulong2num ((unsigned long) entry->uid);
  ve[3] = scm_ulong2num ((unsigned long) entry->gid);
  ve[4] = scm_makfrom0str (entry->gecos);
  if (!entry->dir)
    ve[5] = scm_makfrom0str ("");
  else
    ve[5] = scm_makfrom0str (entry->dir);
  if (!entry->shell)
    ve[6] = scm_makfrom0str ("");
  else
    ve[6] = scm_makfrom0str (entry->shell);
  ve[7] = scm_makfrom0str (entry->mailbox);

  scm_array_handle_release (&handle);
  
  mu_auth_data_free (entry);
  return result;
}
#undef FUNC_NAME

void
mu_scm_mutil_init ()
{
#include "mu_util.x"
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007, 2010, 2011 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#include <mailutils/xcmc.h>

/* Logon - Log on the CMC service.  */

CMC_return_code
cmc_logon (CMC_string service, CMC_string user, CMC_string password,
	   CMC_object_identifier character_set, CMC_ui_id ui_id,
	   CMC_uint16 caller_cmc_version, CMC_flags logon_flags,
	   CMC_session_id * session, CMC_extension * logon_extensions)
{
  return CMC_E_NOT_SUPPORTED;
}

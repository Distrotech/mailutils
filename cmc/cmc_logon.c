/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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

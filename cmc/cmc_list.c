/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <mailutils/xcmc.h>

/* List - list summary information about messages which meet a specified
   criteria.  */
CMC_return_code
cmc_list (CMC_session_id session, CMC_string message_type,
	  CMC_flags list_flags, CMC_message_reference * seed,
	  CMC_uint32 * count, CMC_ui_id ui_id,
	  CMC_message_summary ** result, CMC_extension * list_extensions)
{
  return CMC_E_NOT_SUPPORTED;
}

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <mailutils/xcmc.h>

/* Look Up - Looks up addressing information in the directiory.  */

CMC_return_code
cmc_look_up (CMC_session_id session, CMC_recipient * recipient_in,
	     CMC_flags look_up_flags, CMC_ui_id ui_id, CMC_uint32 * count,
	     CMC_recipient ** recipient_out,
	     CMC_extension *look_up_extensions)
{
  return CMC_E_NOT_SUPPORTED;
}

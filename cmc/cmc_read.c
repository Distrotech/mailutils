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

/* Read - read and return a specified message. */

CMC_return_code
cmc_read (CMC_session_id session, CMC_message_reference *message_reference,
	  CMC_flags read_flags, CMC_message ** message, CMC_ui_id ui_id,
	  CMC_extension * read_extensions)
{
  return CMC_E_NOT_SUPPORTED;
}

/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mailutils/xcmc.h>

/* Read - read and return a specified message. */

CMC_return_code
cmc_read (CMC_session_id session, CMC_message_reference *message_reference,
	  CMC_flags read_flags, CMC_message ** message, CMC_ui_id ui_id,
	  CMC_extension * read_extensions)
{
  return CMC_E_NOT_SUPPORTED;
}

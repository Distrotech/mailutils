/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007, 2010 Free Software Foundation,
   Inc.

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

/* Send Docments - string based function to send mail.  */

CMC_return_code
cmc_send_documents (CMC_string recipient_addresses, CMC_string subject,
		    CMC_string text_note, CMC_flags send_doc_flags,
		    CMC_string file_paths, CMC_string attach_titles,
		    CMC_string delimiter, CMC_ui_id ui_id)
{
  return CMC_E_NOT_SUPPORTED;
}

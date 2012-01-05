/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#include <config.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/sys/imapio.h>

/* If string is NULL, send NIL.
   If it contains \r\n, send it as a literal, replacing 
   contiguous sequences of \r\n by a single space, if UNFOLD is set.
   If string contains " or \, quote it,
   Otherwise send it as is. */

int
mu_imapio_send_qstring_unfold (struct _mu_imapio *io, const char *buffer,
			       int unfold)
{
  int len;
  
  if (buffer == NULL)
    return mu_imapio_printf (io, "NIL");

  if (buffer[len = strcspn (buffer, "\r\n")])
    {
      if (unfold)
	{
	  int rc;
	  size_t size = strlen (buffer);

	  rc = mu_stream_printf (io->_imap_stream,
				 "{%lu}\n", (unsigned long) size);
	  if (rc)
	    return rc;
	  while (1)
	    {
	      mu_stream_write (io->_imap_stream, buffer, len, NULL);
	      buffer += len;
	      if (*buffer)
		{
		  mu_stream_write (io->_imap_stream, " ", 1, NULL);
		  buffer = mu_str_skip_class (buffer, MU_CTYPE_ENDLN);
		  len = strcspn (buffer, "\r\n");
		}
	      else
		break;
	    }
	}
      else
	mu_imapio_send_literal_string (io, buffer);
    }
  else if (buffer[len = strcspn (buffer, io->_imap_ws.ws_escape)])
    {
      int rc;
      
      rc = mu_stream_write (io->_imap_stream, "\"", 1, NULL);
      while (1)
	{
	  mu_stream_write (io->_imap_stream, buffer, len, NULL);
	  buffer += len;
	  if (*buffer)
	    {
	      mu_stream_write (io->_imap_stream, "\\", 1, NULL);
	      mu_stream_write (io->_imap_stream, buffer, 1, NULL);
	      buffer++;
	      len = strcspn (buffer, io->_imap_ws.ws_escape);
	    }
	  else
	    break;
	}
      mu_stream_write (io->_imap_stream, "\"", 1, NULL);
    }
  else if (buffer[0] == 0 || buffer[strcspn (buffer, io->_imap_ws.ws_delim)])
    mu_stream_printf (io->_imap_stream, "\"%s\"", buffer);
  else
    mu_stream_write (io->_imap_stream, buffer, len, NULL);
      
  return mu_stream_last_error (io->_imap_stream);
}

int
mu_imapio_send_qstring (struct _mu_imapio *io, const char *buffer)
{
  return mu_imapio_send_qstring_unfold (io, buffer, 0);
}

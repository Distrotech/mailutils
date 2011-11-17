/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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
#include <mailutils/sys/imapio.h>

/* Send NIL if empty string, change the quoted string to a literal if the
   string contains: double quotes, CR, LF, or \. */

int
mu_imapio_send_qstring_unfold (struct _mu_imapio *io, const char *buffer,
			       int unfold)
{
  if (buffer == NULL || *buffer == '\0')
    return mu_imapio_printf (io, "NIL");
  if (strchr (buffer, '"') ||
      strchr (buffer, '\r') ||
      strchr (buffer, '\n') ||
      strchr (buffer, '\\'))
    {
      if (unfold)
	{
	  int rc;
	  size_t len = strlen (buffer);

	  rc = mu_stream_printf (io->_imap_stream,
				 "{%lu}\n", (unsigned long) strlen (buffer));
	  if (rc)
	    return rc;
	  for (;;)
	    {
	      size_t s = strcspn (buffer, "\r\n");
	      rc = mu_stream_write (io->_imap_stream, buffer, s, NULL);
	      if (rc)
		return rc;
	      len -= s;
	      if (len == 0)
		break;
	      buffer += s;
	      rc = mu_stream_write (io->_imap_stream, " ", 1, NULL);
	      if (rc)
		return rc;
	    }
	}
      else
	return mu_imapio_send_literal (io, buffer);
    }
  return mu_imapio_printf (io, "\"%s\"", buffer);
}

int
mu_imapio_send_qstring (struct _mu_imapio *io, const char *buffer)
{
  return mu_imapio_send_qstring_unfold (io, buffer, 0);
}

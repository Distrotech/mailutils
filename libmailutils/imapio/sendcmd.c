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
#include <stdarg.h>
#include <string.h>
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/sys/imapio.h>

/* Send a IMAP command to the server, quoting its arguments as necessary.
   TAG is the command tag, CMD is the command verb.  Command arguments are
   given in variadic list terminated with NULL. */
int
mu_imapio_send_command (struct _mu_imapio *io, const char *tag,
			char const *cmd, ...)
{
  va_list ap;
  
  va_start (ap, cmd);
  mu_imapio_printf (io, "%s %s", tag, cmd);
  while ((cmd = va_arg (ap, char *)))
    {
      mu_imapio_send (io, " ", 1);
      mu_imapio_send_qstring (io, cmd);
    }
  va_end (ap);
  mu_imapio_send (io, "\r\n", 2);
  return mu_stream_last_error (io->_imap_stream);
}

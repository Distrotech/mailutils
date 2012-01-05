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
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/sys/imapio.h>

/* Send a IMAP command to the server, quoting its arguments as necessary.
   TAG is the command tag, ARGV contains ARGC elements and supplies the
   command (in ARGV[0]) and its arguments. EXTRA (if not NULL) supplies
   additional arguments that will be sent as is.

   If MSGSET is supplied, it is sent when the function encounters the
   "\\" (single backslash) in ARGV.
*/
int
mu_imapio_send_command_v (struct _mu_imapio *io, const char *tag,
			  mu_msgset_t msgset,
			  int argc, char const **argv, const char *extra)
{
  int i;
  
  mu_imapio_printf (io, "%s %s", tag, argv[0]);
  for (i = 1; i < argc; i++)
    {
      mu_imapio_send (io, " ", 1);
      if (msgset && strcmp (argv[i], "\\") == 0)
	mu_imapio_send_msgset (io, msgset);
      else
	mu_imapio_send_qstring (io, argv[i]);
    }
  if (extra)
    {
      mu_imapio_send (io, " ", 1);
      mu_imapio_send (io, extra, strlen (extra));
    }
  mu_imapio_send (io, "\r\n", 2);
  return mu_stream_last_error (io->_imap_stream);
}

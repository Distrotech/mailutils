/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

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

#include "maidag.h"

static void
_sieve_action_log (void *user_name,
		   mu_stream_t stream, size_t msgno,
		   mu_message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  int pfx = 0;

  mu_stream_printf (stream, "\033s<%d>", MU_LOG_NOTICE);
  mu_stream_printf (stream, _("(user %s) "), (char*) user_name);
  if (message_id_header)
    {
      mu_header_t hdr = NULL;
      char *val = NULL;
      mu_message_get_header (msg, &hdr);
      if (mu_header_aget_value (hdr, message_id_header, &val) == 0
	  || mu_header_aget_value (hdr, MU_HEADER_MESSAGE_ID, &val) == 0)
	{
	  pfx = 1;
	  mu_stream_printf (stream, _("%s on msg %s"), action, val);
	  free (val);
	}
    }
  
  if (!pfx)
    {
      size_t uid = 0;
      mu_message_get_uid (msg, &uid);
      mu_stream_printf (stream, _("%s on msg uid %lu"), action,
			(unsigned long) uid);
    }
  
  if (fmt && strlen (fmt))
    {
      mu_stream_printf (stream, "; ");
      mu_stream_vprintf (stream, fmt, ap);
    }
  mu_stream_printf (stream, "\n");
}

int
sieve_check_msg (mu_message_t msg, struct mu_auth_data *auth, const char *prog)
{
  int rc;
  mu_sieve_machine_t mach;

  rc = mu_sieve_machine_init (&mach);
  if (rc)
    {
      mu_error (_("Cannot initialize sieve machine: %s"),
		mu_strerror (rc));
    }
  else
    {
      mu_sieve_set_data (mach, auth->name);
      if (sieve_enable_log)
	mu_sieve_set_logger (mach, _sieve_action_log);
	  
      rc = mu_sieve_compile (mach, prog);
      if (rc == 0)
	mu_sieve_message (mach, msg);
      mu_sieve_machine_destroy (&mach);
    }
  return 0;
}


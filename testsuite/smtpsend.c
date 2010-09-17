/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/mailutils.h>
#include <mailutils/smtp.h>

int
main (int argc, char **argv)
{
  int i;
  char *host;
  int port = 25;
  int trace = 0;
  int tls = 0;
  mu_stream_t stream;
  mu_smtp_t smtp;
  
  mu_set_program_name (argv[0]);
#ifdef WITH_TLS
  mu_init_tls_libs ();
#endif  
  
  if (argc < 2)
    {
      fprintf (stderr, "usage: %s hostname [port=N] [trace=N] [tls=N]\n", argv[0]);
      return 1;
    }

  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "port=", 5) == 0)
	{
	  port = atoi (argv[i] + 5);
	  if (port == 0)
	    {
	      mu_error ("invalid port");
	      return 1;
	    }
	}
      else if (strncmp (argv[i], "trace=", 6) == 0)
	trace = atoi (argv[i] + 6);
      else if (strncmp (argv[i], "tls=", 4) == 0)
	tls = atoi (argv[i] + 4);
      else
	host = argv[i];
    }

  if (!host)
    {
      fprintf (stderr, "usage: %s hostname [port=N] [trace=N]\n", argv[0]);
      return 1;
    }

  
  MU_ASSERT (mu_smtp_create (&smtp));

  host = argv[1];
  MU_ASSERT (mu_tcp_stream_create (&stream, host, port, MU_STREAM_RDWR));
  MU_ASSERT (mu_stream_open (stream));
  mu_smtp_set_carrier (smtp, stream);
  //mu_stream_unref (stream);
  
  if (trace)
    mu_smtp_trace (smtp, MU_SMTP_TRACE_SET);
  MU_ASSERT (mu_smtp_open (smtp));
  MU_ASSERT (mu_smtp_ehlo (smtp));

  if (tls && mu_smtp_capa_test (smtp, "STARTTLS", NULL) == 0)
    {
      MU_ASSERT (mu_smtp_starttls (smtp));
      MU_ASSERT (mu_smtp_ehlo (smtp));
    }
  
  mu_smtp_destroy (&smtp);
  return 0;
}

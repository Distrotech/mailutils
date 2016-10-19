/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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

#include "mu.h"

static int read_recipients;
static mu_address_t rcpt_addr;
static mu_address_t from_addr;

static void
send_address_add (mu_address_t *paddr, const char *value)
{
  mu_address_t addr = NULL;
  int rc;

  rc = mu_address_create (&addr, value);
  if (rc)
    {
      mu_error (_("%s: %s"), value, mu_strerror (rc));
      exit (1);
    }
  MU_ASSERT (mu_address_union (paddr, addr));
  mu_address_destroy (&addr);
}

char send_docstring[] = N_("send a message");
static char send_args_doc[] = "URL-or-HOST [FILE]";

static void
set_from_address (struct mu_parseopt *po, struct mu_option *opt,
		  char const *arg)
{
  MU_ASSERT (mu_address_create_null (&from_addr));
  send_address_add (&from_addr, arg);
}

static void
set_rcpt_address (struct mu_parseopt *po, struct mu_option *opt,
		  char const *arg)
{
  send_address_add (&rcpt_addr, arg);
}

static struct mu_option send_options[] = {
  { "from",  'F', N_("ADDRESS"), MU_OPTION_DEFAULT,
    N_("send mail from this ADDRESS"),
    mu_c_string, NULL, set_from_address },
  { "rcpt",  'T', N_("ADDRESS"), MU_OPTION_DEFAULT,
    N_("send mail to this ADDRESS"),
    mu_c_string, NULL, set_rcpt_address },
  { "read-recipients", 't', NULL, MU_OPTION_DEFAULT,
    N_("read recipients from the message"),
    mu_c_bool, &read_recipients },
  MU_OPTION_END
};

int
mutool_send (int argc, char **argv)
{
  char *infile;
  mu_stream_t instr;
  mu_message_t msg;
  size_t count;
  mu_url_t urlhint, url;
  mu_mailer_t mailer;
  
  MU_ASSERT (mu_address_create_null (&rcpt_addr));
  mu_register_all_mailer_formats ();

  mu_action_getopt (&argc, &argv, send_options, send_docstring, send_args_doc);

  if (argc < 1)
    {
      mu_error (_("not enough arguments"));
      return 1;
    }

  infile = argv[1];
  if (infile)
    MU_ASSERT (mu_file_stream_create (&instr, infile,
				      MU_STREAM_READ|MU_STREAM_SEEK));
  else
    MU_ASSERT (mu_stdio_stream_create (&instr, MU_STDIN_FD,
				       MU_STREAM_READ|MU_STREAM_SEEK));

  MU_ASSERT (mu_stream_to_message (instr, &msg));
  mu_stream_unref (instr);

  mu_address_get_count (rcpt_addr, &count);
  if (count == 0)
    read_recipients = 1;

  if (read_recipients)
    {
      int rc;
      mu_header_t header;
      const char *value;

      MU_ASSERT (mu_message_get_header (msg, &header));
	  
      rc = mu_header_sget_value (header, MU_HEADER_TO, &value);
      if (rc == 0)
	send_address_add (&rcpt_addr, value);
      else if (rc != MU_ERR_NOENT)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_header_sget_value",
			   MU_HEADER_TO, rc);
	  exit (1);
	}
      
      rc = mu_header_sget_value (header, MU_HEADER_CC, &value);
      if (rc == 0)
	send_address_add (&rcpt_addr, value);
      else if (rc != MU_ERR_NOENT)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_header_sget_value",
			   MU_HEADER_CC, rc);
	  exit (1);
	}

      rc = mu_header_sget_value (header, MU_HEADER_BCC, &value);
      if (rc == 0)
	send_address_add (&rcpt_addr, value);
      else if (rc != MU_ERR_NOENT)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_header_sget_value",
			   MU_HEADER_BCC, rc);
	  exit (1);
	}
    }

  mu_address_get_count (rcpt_addr, &count);
  if (count == 0)
    {
      mu_error (_("no recipients"));
      exit (1);
    }
  
  MU_ASSERT (mu_url_create (&urlhint, "smtp://"));
  MU_ASSERT (mu_url_create_hint (&url, argv[0], MU_URL_PARSE_DEFAULT,
				 urlhint));
  mu_url_invalidate (url);
  MU_ASSERT (mu_mailer_create_from_url (&mailer, url));
  MU_ASSERT (mu_mailer_open (mailer, MU_STREAM_RDWR));
  MU_ASSERT (mu_mailer_send_message (mailer, msg, from_addr, rcpt_addr));
  mu_mailer_close (mailer);
  mu_mailer_destroy (&mailer);
  return 0;
}

/*
  MU Setup: send
  mu-handler: mutool_send
  mu-docstring: send_docstring
  End MU Setup:
*/

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
#include <mailutils/cctype.h>
#include <mailutils/mailutils.h>
#include <mailutils/smtp.h>

static char usage_text[] =
"usage: %s hostname [port=N] [trace=N] [tls=N] [from=STRING] [rcpt=STRING]\n"
"                   [domain=STRING] [user=STRING] [pass=STRING]\n"
"                   [service=STRING] [realm=STRING] [host=STRING]\n"
"                   [auth=method[,...]] [url=STRING] [input=FILE] [raw=N]\n"
"                   [skiphdr=name[,...]]\n";

static void
usage ()
{
  fprintf (stderr, usage_text, mu_program_name);
  exit (1);
}

static int
send_rcpt_command (void *item, void *data)
{
  char *email = item;
  mu_smtp_t smtp = data;

  MU_ASSERT (mu_smtp_rcpt_basic (smtp, email, NULL));
  return 0;
}

static void
update_list (mu_list_t *plist, const char *arg)
{
  int mc, j;
  char **mv;
  mu_list_t list = *plist;
  
  if (!list)
    {
      MU_ASSERT (mu_list_create (&list));
      *plist = list;
    }

  MU_ASSERT (mu_argcv_get_np (arg, strlen (arg),
			      ",", NULL,
			      0,
			      &mc, &mv, NULL));
  for (j = 0; j < mc; j++)
    MU_ASSERT (mu_list_append (list, mv[j]));
  free (mv);
}

static int
headercmp (const void *item, const void *data)
{
  return mu_c_strcasecmp (item, data);
}

int
main (int argc, char **argv)
{
  int i;
  char *host = NULL;
  char *infile = NULL;
  int port = 25;
  int tls = 0;
  int raw = 1;
  int flags = 0;
  mu_stream_t stream;
  mu_smtp_t smtp;
  mu_stream_t instr;
  char *from = NULL;
  mu_list_t rcpt_list = NULL;
  mu_list_t meth_list = NULL;
  mu_list_t skiphdr_list = NULL;
  
  mu_set_program_name (argv[0]);
#ifdef WITH_TLS
  mu_init_tls_libs ();
#endif  
  
  if (argc < 2)
    usage ();

  MU_ASSERT (mu_smtp_create (&smtp));

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
	{
	  char *arg = argv[i] + 6;

	  if (mu_isdigit (arg[0]))
	    mu_smtp_trace (smtp, atoi (argv[i] + 6) ?
			   MU_SMTP_TRACE_SET : MU_SMTP_TRACE_CLR);
	  else if (strcmp (arg, "secure") == 0)
	    mu_smtp_trace_mask (smtp, MU_SMTP_TRACE_SET, MU_XSCRIPT_SECURE);
	  else if (strcmp (arg, "payload") == 0)
	    mu_smtp_trace_mask (smtp, MU_SMTP_TRACE_SET, MU_XSCRIPT_PAYLOAD);
	}
      else if (strncmp (argv[i], "tls=", 4) == 0)
	tls = atoi (argv[i] + 4);
      else if (strncmp (argv[i], "domain=", 7) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_DOMAIN,
				      argv[i] + 7));
      else if (strncmp (argv[i], "user=", 5) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_USERNAME,
				      argv[i] + 5));
      else if (strncmp (argv[i], "pass=", 5) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_PASSWORD,
				      argv[i] + 5));
      else if (strncmp (argv[i], "service=", 8) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_SERVICE,
				      argv[i] + 8));
      else if (strncmp (argv[i], "realm=", 6) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_REALM,
				      argv[i] + 6));
      else if (strncmp (argv[i], "host=", 5) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_HOST,
				      argv[i] + 5));
      else if (strncmp (argv[i], "url=", 4) == 0)
	MU_ASSERT (mu_smtp_set_param (smtp, MU_SMTP_PARAM_URL,
				      argv[i] + 4));
      else if (strncmp (argv[i], "input=", 6) == 0)
	infile = argv[i] + 6;
      else if (strncmp (argv[i], "raw=", 4) == 0)
	raw = atoi (argv[i] + 4);
      else if (strncmp (argv[i], "rcpt=", 5) == 0)
	{
	  if (!rcpt_list)
	    MU_ASSERT (mu_list_create (&rcpt_list));
	  MU_ASSERT (mu_list_append (rcpt_list, argv[i] + 5));
	}
      else if (strncmp (argv[i], "from=", 5) == 0)
	from = argv[i] + 5;
      else if (strncmp (argv[i], "auth=", 5) == 0)
	update_list (&meth_list, argv[i] + 5);
      else if (strncmp (argv[i], "skiphdr=", 8) == 0)
	{
	  update_list (&skiphdr_list, argv[i] + 8);
	  raw = 0;
	}
      else if (host)
	{
	  mu_error ("server name already given: %s, new name %s?",
		    host, argv[i]);
	  exit (1);
	}
      else
	host = argv[i];
    }

  if (!host)
    usage ();

  if (!raw)
    flags = MU_STREAM_SEEK;
  if (infile)
    {
      MU_ASSERT (mu_file_stream_create (&instr, infile, MU_STREAM_READ|flags));
      MU_ASSERT (mu_stream_open (instr));
    }
  else
    MU_ASSERT (mu_stdio_stream_create (&instr, MU_STDIN_FD, flags));
  
  host = argv[1];
  MU_ASSERT (mu_tcp_stream_create (&stream, host, port, MU_STREAM_RDWR));
  MU_ASSERT (mu_stream_open (stream));
  mu_smtp_set_carrier (smtp, stream);
  mu_stream_unref (stream);
  
  if (!from)
    {
      from = getenv ("USER");
      if (!from)
	{
	  mu_error ("cannot determine sender name");
	  exit (1);
	}
    }

  if (raw && !rcpt_list)
    {
      mu_error ("no recipients");
      exit (1);
    }
  
  MU_ASSERT (mu_smtp_open (smtp));
  MU_ASSERT (mu_smtp_ehlo (smtp));

  if (tls && mu_smtp_capa_test (smtp, "STARTTLS", NULL) == 0)
    {
      MU_ASSERT (mu_smtp_starttls (smtp));
      MU_ASSERT (mu_smtp_ehlo (smtp));
    }

  if (meth_list)
    {
      int status;
      
      MU_ASSERT (mu_smtp_add_auth_mech_list (smtp, meth_list));
      status = mu_smtp_auth (smtp);
      switch (status)
	{
	case 0:
	  MU_ASSERT (mu_smtp_ehlo (smtp));
	  break;
	  
	case ENOSYS:
	case MU_ERR_NOENT:
	  /* Ok, skip it */
	  break;

	default:
	  mu_error ("authentication failed: %s", mu_strerror (status));
	  exit (1);
	}
    }
  
  MU_ASSERT (mu_smtp_mail_basic (smtp, from, NULL));
  mu_list_do (rcpt_list, send_rcpt_command, smtp);
  
  if (raw)
    {
      /* Raw sending mode: send from the stream directly */
      MU_ASSERT (mu_smtp_send_stream (smtp, instr));
    }
  else
    {
      /* Message (standard) sending mode: send a MU message. */

      mu_message_t msg;
      mu_stream_t ostr, bstr;
      mu_header_t hdr;
      mu_iterator_t itr;
      mu_body_t body;

      if (skiphdr_list)
	mu_list_set_comparator (skiphdr_list, headercmp);
      
      MU_ASSERT (mu_stream_to_message (instr, &msg));
      MU_ASSERT (mu_smtp_data (smtp, &ostr));
      MU_ASSERT (mu_message_get_header (msg, &hdr));
      MU_ASSERT (mu_header_get_iterator (hdr, &itr));
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  const char *name;
	  void *value;

	  mu_iterator_current_kv (itr, (void*) &name, &value);
	  if (mu_list_locate (skiphdr_list, (void*) name, NULL) == 0)
	    continue;

	  mu_stream_printf (ostr, "%s: %s\n", name, (char*)value);
	}
      mu_iterator_destroy (&itr);
      MU_ASSERT (mu_stream_write (ostr, "\n", 1, NULL));
      
      MU_ASSERT (mu_message_get_body (msg, &body));
      MU_ASSERT (mu_body_get_streamref (body, &bstr));
      MU_ASSERT (mu_stream_copy (ostr, bstr, 0, NULL));
      mu_stream_destroy (&bstr);
      mu_stream_close (ostr);
      mu_stream_destroy (&ostr);
    }
  
  MU_ASSERT (mu_smtp_quit (smtp));
  
  mu_smtp_destroy (&smtp);
  mu_stream_close (instr);
  mu_stream_destroy (&instr);
  return 0;
}

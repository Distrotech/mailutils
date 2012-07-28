/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mailutils/mailutils.h>
#include "mu.h"
#include "argp.h"

static char pop_doc[] = N_("mu pop - POP3 client shell.");
char pop_docstring[] = N_("POP3 client shell");
static char pop_args_doc[] = "";

static struct argp_option pop_options[] = {
  { NULL }
};

static error_t
pop_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp pop_argp = {
  pop_options,
  pop_parse_opt,
  pop_args_doc,
  pop_doc,
  NULL,
  NULL,
  NULL
};

/* Global handle for pop3.  */
static mu_pop3_t pop3;

enum pop_session_status
  {
    pop_session_disconnected,
    pop_session_connected,
    pop_session_logged_in
  };

static enum pop_session_status pop_session_status;

static int connect_argc;
static char **connect_argv;

/* Host we are connected to. */
#define host connect_argv[0]
static int port = 110;
static char *username;

const char *
pop_session_str (enum pop_session_status stat)
{
  switch (stat)
    {
    case pop_session_disconnected:
      return "disconnected";
      
    case pop_session_connected:
      return "connected";
      
    case pop_session_logged_in:
      return "logged in";
    }
  return "unknown";
}

static void
pop_prompt_env ()
{
  if (!mutool_prompt_env)
    mutool_prompt_env = mu_calloc (2*7 + 1, sizeof(mutool_prompt_env[0]));

  mutool_prompt_env[0] = "user";
  mutool_prompt_env[1] = (pop_session_status == pop_session_logged_in) ?
                           username : "[nouser]";

  mutool_prompt_env[2] = "host"; 
  mutool_prompt_env[3] = (pop_session_status != pop_session_disconnected) ?
                           host : "[nohost]";

  mutool_prompt_env[4] = "program-name";
  mutool_prompt_env[5] = (char*) mu_program_name;

  mutool_prompt_env[6] = "canonical-program-name";
  mutool_prompt_env[7] = "mu";

  mutool_prompt_env[8] = "package";
  mutool_prompt_env[9] = PACKAGE;

  mutool_prompt_env[10] = "version";
  mutool_prompt_env[11] = PACKAGE_VERSION;

  mutool_prompt_env[12] = "status";
  mutool_prompt_env[13] = (char*) pop_session_str (pop_session_status);

  mutool_prompt_env[14] = NULL;
}


static void
pop_set_verbose (void)
{
  if (pop3)
    {
      if (QRY_VERBOSE ())
	mu_pop3_trace (pop3, MU_POP3_TRACE_SET);
      else
	mu_pop3_trace (pop3, MU_POP3_TRACE_CLR);
    }
}

static void
pop_set_verbose_mask (void)
{
  if (pop3)
    {
      mu_pop3_trace_mask (pop3, QRY_VERBOSE_MASK (MU_XSCRIPT_SECURE)
			          ? MU_POP3_TRACE_SET : MU_POP3_TRACE_CLR,
			      MU_XSCRIPT_SECURE);
      mu_pop3_trace_mask (pop3, QRY_VERBOSE_MASK (MU_XSCRIPT_PAYLOAD)
			          ? MU_POP3_TRACE_SET : MU_POP3_TRACE_CLR,
			      MU_XSCRIPT_PAYLOAD);
    }
}

static int
com_verbose (int argc, char **argv)
{
  return shell_verbose (argc, argv,
			pop_set_verbose, pop_set_verbose_mask);
}

static int
com_user (int argc, char **argv)
{
  int status;
  
  status = mu_pop3_user (pop3, argv[1]);
  if (status == 0)
    {
      username = mu_strdup (argv[1]);
      pop_prompt_env ();
    }
  return status;
}

static int
com_apop (int argc, char **argv)
{
  int status;
  char *pwd, *passbuf = NULL;

  if (argc == 3)
    pwd = argv[2];
  else if (!mutool_shell_interactive)
    {
      mu_error (_("apop: password required"));
      return 1;
    }
  else
    {
      status = mu_getpass (mu_strin, mu_strout, "Password:", &passbuf);
      if (status)
	return status;
      pwd = passbuf;
    }
  
  status = mu_pop3_apop (pop3, argv[1], pwd);
  if (status == 0)
    {
      username = mu_strdup (argv[1]);
      pop_session_status = pop_session_logged_in;
    }
  free (passbuf);
  return status;
}

static int
com_capa (int argc, char **argv)
{
  mu_iterator_t iterator = NULL;
  int status = 0;
  int reread = 0;
  int i = 1;
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-reread") == 0)
	reread = 1;
      else
	break;
    }

  if (i < argc)
    {
      if (reread)
	{
	  status = mu_pop3_capa (pop3, 1, NULL);
	  if (status)
	    return status;
	}
      for (; i < argc; i++)
	{
	  const char *elt;
	  int rc = mu_pop3_capa_test (pop3, argv[i], &elt);
	  switch (rc)
	    {
	    case 0:
	      if (*elt)
		mu_printf ("%s: %s\n", argv[i], elt);
	      else
		mu_printf ("%s is set\n", argv[i]);
	      break;

	    case MU_ERR_NOENT:
	      mu_printf ("%s is not set\n", argv[i]);
	      break;

	    default:
	      return rc;
	    }
	}
    }
  else
    {
      status = mu_pop3_capa (pop3, reread, &iterator);

      if (status == 0)
	{
	  for (mu_iterator_first (iterator);
	       !mu_iterator_is_done (iterator); mu_iterator_next (iterator))
	    {
	      char *capa = NULL;
	      mu_iterator_current (iterator, (void **) &capa);
	      mu_printf ("CAPA: %s\n", capa ? capa : "");
	    }
	  mu_iterator_destroy (&iterator);
	}
    }
  return status;
}

static int
com_uidl (int argc, char **argv)
{
  int status = 0;
  if (argc == 1)
    {
      mu_stream_t out = mutool_open_pager ();
      mu_iterator_t uidl_iterator = NULL;
      status = mu_pop3_uidl_all (pop3, &uidl_iterator);
      if (status == 0)
	{
	  for (mu_iterator_first (uidl_iterator);
	       !mu_iterator_is_done (uidl_iterator);
	       mu_iterator_next (uidl_iterator))
	    {
	      char *uidl = NULL;
	      mu_iterator_current (uidl_iterator, (void **) &uidl);
	      mu_stream_printf (out, "UIDL: %s\n", uidl ? uidl : "");
	    }
	  mu_iterator_destroy (&uidl_iterator);
	}
      mu_stream_destroy (&out);
    }
  else
    {
      char *uidl = NULL;
      unsigned int msgno = strtoul (argv[1], NULL, 10);
      status = mu_pop3_uidl (pop3, msgno, &uidl);
      if (status == 0)
	mu_printf ("Msg: %d UIDL: %s\n", msgno, uidl ? uidl : "");
      free (uidl);
    }
  return status;
}

static int
com_list (int argc, char **argv)
{
  int status = 0;
  if (argc == 1)
    {
      mu_stream_t out = mutool_open_pager ();
      mu_iterator_t list_iterator;
      status = mu_pop3_list_all (pop3, &list_iterator);
      if (status == 0)
	{
	  for (mu_iterator_first (list_iterator);
	       !mu_iterator_is_done (list_iterator);
	       mu_iterator_next (list_iterator))
	    {
	      char *list = NULL;
	      mu_iterator_current (list_iterator, (void **) &list);
	      mu_stream_printf (out, "LIST: %s\n", (list) ? list : "");
	    }
	  mu_iterator_destroy (&list_iterator);
	  mu_stream_destroy (&out);
	}
    }
  else
    {
      size_t size = 0;
      unsigned int msgno = strtoul (argv[1], NULL, 10);
      status = mu_pop3_list (pop3, msgno, &size);
      if (status == 0)
	mu_printf ("Msg: %u Size: %lu\n", msgno, (unsigned long) size);
    }
  return status;
}

static int
com_noop (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  return mu_pop3_noop (pop3);
}

static int
com_pass (int argc, char **argv)
{
  int status;
  char *pwd, *passbuf = NULL;
  
  if (argc == 1)
    {
      if (!mutool_shell_interactive)
	{
	  mu_error (_("pass: password required"));
	  return 1;
	}
      status = mu_getpass (mu_strin, mu_strout, "Password:", &passbuf);
      if (status)
	return status;
      pwd = passbuf;
    }
  else
    pwd = argv[1];
  status = mu_pop3_pass (pop3, pwd);
  if (status == 0)
    {
      pop_session_status = pop_session_logged_in;
      pop_prompt_env ();
    }
  free (passbuf);
  return status;
}

static int
com_stat (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  size_t count = 0;
  mu_off_t size = 0;
  int status = 0;

  status = mu_pop3_stat (pop3, &count, &size);
  mu_printf ("Mesgs: %lu Size %lu\n",
	     (unsigned long) count, (unsigned long) size);
  return status;
}

static int
com_stls (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  return mu_pop3_stls (pop3);
}

static int
com_dele (int argc, char **argv)
{
  unsigned msgno;
  msgno = strtoul (argv[1], NULL, 10);
  return mu_pop3_dele (pop3, msgno);
}

static int
com_rset (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  return mu_pop3_rset (pop3);
}

static int
com_top (int argc, char **argv)
{
  mu_stream_t stream;
  unsigned int msgno;
  unsigned int lines;
  int status;

  msgno = strtoul (argv[1], NULL, 10);
  if (argc == 3)
    lines = strtoul (argv[2], NULL, 10);
  else
    lines = 5;

  status = mu_pop3_top (pop3, msgno, lines, &stream);

  if (status == 0)
    {
      mu_stream_t out = mutool_open_pager ();
      mu_stream_copy (out, stream, 0, NULL);
      mu_stream_destroy (&out);
      mu_stream_destroy (&stream);
    }
  return status;
}

static int
com_retr (int argc, char **argv)
{
  mu_stream_t stream;
  unsigned int msgno;
  int status;

  msgno = strtoul (argv[1], NULL, 10);
  status = mu_pop3_retr (pop3, msgno, &stream);

  if (status == 0)
    {
      mu_stream_t out = mutool_open_pager ();
      mu_stream_copy (out, stream, 0, NULL);
      mu_stream_destroy (&out);
      mu_stream_destroy (&stream);
    }
  return status;
}

static int
com_disconnect (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  if (pop3)
    {
      mu_pop3_disconnect (pop3);
      mu_pop3_destroy (&pop3);
      pop3 = NULL;
      
      mu_argcv_free (connect_argc, connect_argv);
      connect_argc = 0;
      connect_argv = NULL;
      pop_session_status = pop_session_disconnected;
      pop_prompt_env ();
    }
  return 0;
}

static int
com_connect (int argc, char **argv)
{
  int status;
  int tls = 0;
  int i = 1;
  int n;
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-tls") == 0)
	{
#ifdef WITH_TLS
	    tls = 1;
#else
	    mu_error ("TLS not supported");
	    return 0;
#endif
	}
      else
	break;
    }

  argc -= i;
  argv += i;
  
  if (pop_session_status != pop_session_disconnected)
    com_disconnect (0, NULL);
  
  status = mu_pop3_create (&pop3);
  if (status == 0)
    {
      mu_stream_t tcp;
      struct mu_sockaddr *sa;
      struct mu_sockaddr_hints hints;

      if (QRY_VERBOSE ())
	{
	  pop_set_verbose ();
	  pop_set_verbose_mask ();
	}

      memset (&hints, 0, sizeof (hints));
      hints.flags = MU_AH_DETECT_FAMILY;
      hints.port = tls ? MU_POP3_DEFAULT_SSL_PORT : MU_POP3_DEFAULT_PORT;
      hints.protocol = IPPROTO_TCP;
      hints.socktype = SOCK_STREAM;
      status = mu_sockaddr_from_node (&sa, argv[0], argv[1], &hints);
      if (status == 0)
	{
	  n = port_from_sa (sa);
	  status = mu_tcp_stream_create_from_sa (&tcp, sa, NULL, 0);
	  if (status)
	    mu_sockaddr_free (sa);
	}
      if (status == 0)
	{
#ifdef WITH_TLS
	  if (tls)
	    {
	      mu_stream_t tlsstream;
	      
	      status = mu_tls_client_stream_create (&tlsstream, tcp, tcp, 0);
	      mu_stream_unref (tcp);
	      if (status)
		{
		  mu_error ("cannot create TLS stream: %s",
			    mu_strerror (status));
		  return 0;
		}
	      tcp = tlsstream;
	    }
#endif
	  mu_pop3_set_carrier (pop3, tcp);
	  status = mu_pop3_connect (pop3);
	}
      else
	{
	  mu_pop3_destroy (&pop3);
	  pop3 = NULL;
	}
    }

  if (status)
    mu_error ("Failed to create pop3: %s", mu_strerror (status));
  else
    {
      connect_argc = argc;
      connect_argv = mu_calloc (argc, sizeof (*connect_argv));
      for (i = 0; i < argc; i++)
	connect_argv[i] = mu_strdup (argv[i]);
      connect_argv[i] = NULL;
      port = n;
      pop_session_status = pop_session_connected;

      pop_prompt_env ();
    }
  
  return status;
}

static int
com_quit (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = 0;
  if (pop3)
    {
      if (mu_pop3_quit (pop3) == 0)
	{
	  status = com_disconnect (0, NULL);
	}
      else
	{
	  mu_printf ("Try 'exit' to leave %s\n", mu_program_name);
	}
    }
  else
    mu_printf ("Try 'exit' to leave %s\n", mu_program_name);
  return status;
}

struct mutool_command pop_comtab[] = {
  { "apop",       2,  3, 0, com_apop,
    N_("USER [PASS]"),
    N_("authenticate with APOP") },
  { "capa",       1, -1, 0, com_capa,
    /* TRANSLATORS: -reread is a keyword; do not translate. */
    N_("[-reread] [NAME...]"),
    N_("list server capabilities") },
  { "disconnect", 1, 1, 0,
    com_disconnect,
    NULL,
    N_("close connection") },
  { "dele",       2, 2, 0, com_dele,
    N_("NUMBER"),
    N_("mark message for deletion") },
  { "list",       1, 2, 0, com_list,
    N_("[NUMBER]"),
    N_("list messages") },
  { "noop",       1, 1, 0, com_noop,
    NULL,
    N_("send a \"no operation\"") },
  { "pass",       1, 2, 0, com_pass,
    N_("[PASSWORD]"),
    N_("send password") },
  { "connect",    1, 4, 0, com_connect,
    /* TRANSLATORS: --tls is a keyword. */
    N_("[-tls] HOSTNAME [PORT]"),
    N_("open connection") },
  { "quit",       1, 1, 0, com_quit,
    NULL,
    N_("quit pop3 session") },
  { "retr",       2, 2, 0, com_retr,
    "NUMBER",
    N_("retrieve a message") },
  { "rset",       1, 1, 0, com_rset,
    NULL,
    N_("remove deletion marks") },
  { "stat",       1, 1, 0, com_stat,
    NULL,
    N_("get the mailbox size and number of messages in it") },
  { "stls",       1, 1, 0, com_stls,
    NULL,
    N_("start TLS negotiation") },
  { "top",        2, 3, 0, com_top,
    "MSGNO [NUMBER]",
    N_("display message headers and first NUMBER (default 5) lines of"
       " its body") },
  { "uidl",       1, 2, 0, com_uidl,
    N_("[NUMBER]"),
    N_("show unique message identifiers") },
  { "user",       2, 2, 0, com_user,
    N_("NAME"),
    N_("send login") },
  { "verbose",    1, 4, 0, com_verbose,
    "[on|off|mask|unmask] [secure [payload]]",
    N_("control the protocol tracing") },
  { NULL }
};


int
mutool_pop (int argc, char **argv)
{
  int index;

  if (argp_parse (&pop_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;
  
  argc -= index;
  argv += index;

  if (argc)
    {
      mu_error (_("too many arguments"));
      return 1;
    }

  /* Command line prompt */
  mutool_shell_prompt = mu_strdup ("pop> ");
  pop_prompt_env ();
  mutool_shell ("pop", pop_comtab);
  return 0;
}

/*
  MU Setup: pop
  mu-handler: mutool_pop
  mu-docstring: pop_docstring
  mu-cond: ENABLE_POP
  End MU Setup:
*/

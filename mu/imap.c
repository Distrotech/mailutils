/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mailutils/mailutils.h>
#include <mailutils/imap.h>
#include <mailutils/imapio.h>
#include "mu.h"
#include "argp.h"
#include "xalloc.h"

static char imap_doc[] = N_("mu imap - IMAP4 client shell.");
char imap_docstring[] = N_("IMAP4 client shell");
static char imap_args_doc[] = "";

static struct argp_option imap_options[] = {
  { NULL }
};

static error_t
imap_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp imap_argp = {
  imap_options,
  imap_parse_opt,
  imap_args_doc,
  imap_doc,
  NULL,
  NULL,
  NULL
};


static mu_imap_t imap;

static enum mu_imap_state
current_imap_state ()
{
  int state;
  if (imap == NULL)
    state = MU_IMAP_STATE_INIT;
  else
    {
      mu_imap_state (imap, &state);
      if (state == MU_IMAP_STATE_LOGOUT)
	state = MU_IMAP_STATE_INIT;
    }
  return state;
}


static void
imap_set_verbose ()
{
  if (imap)
    {
      if (QRY_VERBOSE ())
	mu_imap_trace (imap, MU_IMAP_TRACE_SET);
      else
	mu_imap_trace (imap, MU_IMAP_TRACE_CLR);
    }
}

void
imap_set_verbose_mask ()
{
  if (imap)
    {
      mu_imap_trace_mask (imap, QRY_VERBOSE_MASK (MU_XSCRIPT_SECURE)
			          ? MU_IMAP_TRACE_SET : MU_IMAP_TRACE_CLR,
			      MU_XSCRIPT_SECURE);
      mu_imap_trace_mask (imap, QRY_VERBOSE_MASK (MU_XSCRIPT_PAYLOAD)
			          ? MU_IMAP_TRACE_SET : MU_IMAP_TRACE_CLR,
			      MU_XSCRIPT_PAYLOAD);
    }
}

static int
com_verbose (int argc, char **argv)
{
  return shell_verbose (argc, argv,
			imap_set_verbose, imap_set_verbose_mask);
  
}

static int connect_argc;
static char **connect_argv;
#define host connect_argv[0]

static char *username;

static void
imap_prompt_env ()
{
  enum mu_imap_state state = current_imap_state ();
  if (!mutool_prompt_env)
    mutool_prompt_env = xcalloc (2*7 + 1, sizeof(mutool_prompt_env[0]));

  mutool_prompt_env[0] = "user";
  mutool_prompt_env[1] = (state >= MU_IMAP_STATE_AUTH && username) ?
                           username : "[nouser]";

  mutool_prompt_env[2] = "host"; 
  mutool_prompt_env[3] = connect_argv ? host : "[nohost]";

  mutool_prompt_env[4] = "program-name";
  mutool_prompt_env[5] = (char*) mu_program_name;

  mutool_prompt_env[6] = "canonical-program-name";
  mutool_prompt_env[7] = "mu";

  mutool_prompt_env[8] = "package";
  mutool_prompt_env[9] = PACKAGE;

  mutool_prompt_env[10] = "version";
  mutool_prompt_env[11] = PACKAGE_VERSION;

  mutool_prompt_env[12] = "status";
  if (mu_imap_state_str (state, (const char **) &mutool_prompt_env[13]))
    mutool_prompt_env[12] = NULL;

  mutool_prompt_env[14] = NULL;
}

/* Callbacks */
static void
imap_popauth_callback (void *data, int code, va_list ap)
{
  int rcode = va_arg (ap, int);
  const char *text = va_arg (ap, const char *);
  if (text)
    mu_diag_output (MU_DIAG_INFO, _("session authenticated: %s"), text);
  else
    mu_diag_output (MU_DIAG_INFO, _("session authenticated"));
}

static int
com_disconnect (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  if (imap)
    {
      mu_imap_disconnect (imap);
      mu_imap_destroy (&imap);
      
      mu_argcv_free (connect_argc, connect_argv);
      connect_argc = 0;
      connect_argv = NULL;
      imap_prompt_env ();
    }
  return 0;
}

static int
com_connect (int argc, char **argv)
{
  int status;
  int tls = 0;
  int i = 1;
  enum mu_imap_state state;
  
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
  
  state = current_imap_state ();
  
  if (state != MU_IMAP_STATE_INIT)
    com_disconnect (0, NULL);
  
  status = mu_imap_create (&imap);
  if (status == 0)
    {
      mu_stream_t tcp;
      struct mu_sockaddr *sa;
      struct mu_sockaddr_hints hints;

      memset (&hints, 0, sizeof (hints));
      hints.flags = MU_AH_DETECT_FAMILY;
      hints.port = tls ? MU_IMAP_DEFAULT_SSL_PORT : MU_IMAP_DEFAULT_PORT;
      hints.protocol = IPPROTO_TCP;
      hints.socktype = SOCK_STREAM;
      status = mu_sockaddr_from_node (&sa, argv[0], argv[1], &hints);
      if (status == 0)
        {
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
	  mu_imap_set_carrier (imap, tcp);

	  if (QRY_VERBOSE ())
	    {
	      imap_set_verbose ();
	      imap_set_verbose_mask ();
	    }

	  /* Set callbacks */
	  mu_imap_register_callback_function (imap, MU_IMAP_CB_PREAUTH,
					      imap_popauth_callback,
					      NULL);

	  
	  status = mu_imap_connect (imap);
	  if (status)
	    {
	      const char *err;

	      mu_error ("Failed to connect: %s", mu_strerror (status));
	      if (mu_imap_strerror (imap, &err))
		mu_error ("server response: %s", err);
	      mu_imap_destroy (&imap);
	    }
	}
      else
	mu_imap_destroy (&imap);
    }
  else
    mu_error ("Failed to create imap: %s", mu_strerror (status));
  
  if (!status)
    {
      connect_argc = argc;
      connect_argv = xcalloc (argc, sizeof (*connect_argv));
      for (i = 0; i < argc; i++)
	connect_argv[i] = xstrdup (argv[i]);
      connect_argv[i] = NULL;
    
      imap_prompt_env ();
    }
  
  return status;
}

static int
com_logout (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = 0;
  if (imap)
    {
      if (mu_imap_logout (imap) == 0)
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

static int
com_capability (int argc, char **argv)
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
	  status = mu_imap_capability (imap, 1, NULL);
	  if (status)
	    return status;
	}
      for (; i < argc; i++)
	{
	  const char *elt;
	  int rc = mu_imap_capability_test (imap, argv[i], &elt);
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
      status = mu_imap_capability (imap, reread, &iterator);

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
com_login (int argc, char **argv)
{
  int status;
  char *pwd, *passbuf = NULL;

  if (!imap)
    {
      mu_error (_("you need to connect first"));
      return 0;
    }
  
  if (argc == 2)
    {
      if (!mutool_shell_interactive)
	{
	  mu_error (_("login: password required"));
	  return 1;
	}
      status = mu_getpass (mu_strin, mu_strout, "Password:", &passbuf);
      if (status)
	return status;
      pwd = passbuf;
    }
  else
    pwd = argv[2];

  status = mu_imap_login (imap, argv[1], pwd);
  memset (pwd, 0, strlen (pwd));
  free (passbuf);
  if (status == 0)
    imap_prompt_env ();
  else
    {
      const char *str;
      
      mu_error ("authentication failed: %s", mu_strerror (status));
      if (mu_imap_strerror (imap, &str) == 0)
	mu_error ("server reply: %s", str);
    }
  return 0;
}

static int
com_id (int argc, char **argv)
{
  mu_assoc_t assoc;
  char *test = NULL;
  int status;

  argv++;
  if (argv[0] && strcmp (argv[0], "-test") == 0)
    {
      argv++;
      if (argv[0] == NULL)
	{
	  mu_error ("id -test requires an argument");
	  return 0;
	}
      test = argv[0];
      argv++;
    }
  
  status = mu_imap_id (imap, argv + 1, &assoc);
  if (status == 0)
    {
      if (test)
	{
	  void *res = mu_assoc_ref (assoc, test);
	  if (res)
	    {
	      mu_printf ("%s: %s\n", test, *(char **)res);
	    }
	  else
	    mu_printf ("%s is not set\n", test);
	}
      else
	{
	  mu_iterator_t itr;
	  
	  mu_assoc_get_iterator (assoc, &itr);
	  for (mu_iterator_first (itr);
	       !mu_iterator_is_done (itr); mu_iterator_next (itr))
	    {
	      char *key;
	      void *val;

	      mu_iterator_current_kv (itr, (const void**)&key, &val);
	      mu_printf ("ID: %s %s\n", key, *(char**)val);
	    }
	  mu_iterator_destroy (&itr);
	}
      mu_assoc_destroy (&assoc);
    }
  return status;
}

static void
print_imap_stats (struct mu_imap_stat *st)
{
  if (st->flags & MU_IMAP_STAT_DEFINED_FLAGS)
    {
      mu_printf (_("Flags defined: "));
      mu_imap_format_flags (mu_strout, st->defined_flags);
      mu_printf ("\n");
    }
  if (st->flags & MU_IMAP_STAT_PERMANENT_FLAGS)
    {
      mu_printf (_("Flags permanent: "));
      mu_imap_format_flags (mu_strout, st->permanent_flags);
      mu_printf ("\n");
    }

  if (st->flags & MU_IMAP_STAT_MESSAGE_COUNT)
    mu_printf (_("Total messages: %lu\n"), (unsigned long) st->message_count);
  if (st->flags & MU_IMAP_STAT_RECENT_COUNT)
    mu_printf (_("Recent messages: %lu\n"), (unsigned long) st->recent_count);
  if (st->flags & MU_IMAP_STAT_FIRST_UNSEEN)
    mu_printf (_("First unseen message: %lu\n"),
	       (unsigned long) st->first_unseen);
  if (st->flags & MU_IMAP_STAT_UIDNEXT)
    mu_printf (_("Next UID: %lu\n"), (unsigned long) st->uidnext);
  if (st->flags & MU_IMAP_STAT_UIDVALIDITY)
    mu_printf (_("UID validity: %lu\n"), st->uidvalidity);
}


static int
select_mbox (int argc, char **argv, int writable)
{
  int status;
  struct mu_imap_stat st;

  status = mu_imap_select (imap, argv[1], writable, &st);
  if (status == 0)
    {
      print_imap_stats (&st);
      imap_prompt_env ();
    }
  else
    {
      const char *str;
      
      mu_error ("select failed: %s", mu_strerror (status));
      if (mu_imap_strerror (imap, &str) == 0)
	mu_error ("server reply: %s", str);
    }
  return 0;
}

static int
com_select (int argc, char **argv)
{
  return select_mbox (argc, argv, 1);
}

static int
com_examine (int argc, char **argv)
{
  return select_mbox (argc, argv, 0);
}

static int
com_status (int argc, char **argv)
{
  struct mu_imap_stat st;
  int i, flag;
  int status;
    
  st.flags = 0;
  for (i = 2; i < argc; i++)
    {
      if (mu_kwd_xlat_name_ci (_mu_imap_status_name_table, argv[i], &flag))
	{
	  mu_error (_("unknown data item: %s"), argv[i]);
	  return 0;
	}
      st.flags |= flag;
    }

  status = mu_imap_status (imap, argv[1], &st);
  if (status == 0)
    {
      print_imap_stats (&st);
    }
  else
    {
      const char *str;
      
      mu_error ("status failed: %s", mu_strerror (status));
      if (mu_imap_strerror (imap, &str) == 0)
	mu_error ("server reply: %s", str);
    }
  return 0;
}

struct mutool_command imap_comtab[] = {
  { "capability", 1, -1, com_capability,
    /* TRANSLATORS: -reread is a keyword; do not translate. */
    N_("[-reread] [NAME...]"),
    N_("list server capabilities") },
  { "verbose",    1, 4, com_verbose,
    "[on|off|mask|unmask] [secure [payload]]",
    N_("control the protocol tracing") },
  { "connect",    1, 4, com_connect,
    /* TRANSLATORS: --tls is a keyword. */
    N_("[-tls] HOSTNAME [PORT]"),
    N_("open connection") },
  { "disconnect", 1, 1,
    com_disconnect,
    NULL,
    N_("close connection") },
  { "login",        2, 3, com_login,
    N_("USER [PASS]"),
    N_("login to the server") },
  { "logout",       1, 1, com_logout,
    NULL,
    N_("quit imap session") },
  { "id",           1, -1, com_id,
    N_("[-test KW] [ARG [ARG...]]"),
    N_("send ID command") },
  { "select",       1, 2, com_select,
    N_("[MBOX]"),
    N_("select a mailbox") },
  { "examine",      1, 2, com_examine,
    N_("[MBOX]"),
    N_("examine a mailbox") },
  { "status",       3, -1, com_status,
    N_("MBOX KW [KW...]"),
    N_("get mailbox status") },
  { "quit",         1, 1, com_logout,
    NULL,
    N_("same as `logout'") },
  { NULL }
};

int
mutool_imap (int argc, char **argv)
{
  int index;

  if (argp_parse (&imap_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

  if (argc)
    {
      mu_error (_("too many arguments"));
      return 1;
    }

  /* Command line prompt */
  mutool_shell_prompt = xstrdup ("imap> ");
  imap_prompt_env ();
  mutool_shell ("imap", imap_comtab);
  return 0;
}

/*
  MU Setup: imap
  mu-handler: mutool_imap
  mu-docstring: imap_docstring
  mu-cond: ENABLE_IMAP
  End MU Setup:
*/

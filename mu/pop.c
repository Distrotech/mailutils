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
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <mailutils/mailutils.h>
#include "mu.h"
#include "argp.h"
#include "xalloc.h"

static char pop_doc[] = N_("mu pop - POP3 client shell.");
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

int
get_bool (const char *str, int *pb)
{
  if (mu_c_strcasecmp (str, "yes") == 0
      || mu_c_strcasecmp (str, "on") == 0
      || mu_c_strcasecmp (str, "true") == 0)
    *pb = 1;
  else if (mu_c_strcasecmp (str, "no") == 0
      || mu_c_strcasecmp (str, "off") == 0
      || mu_c_strcasecmp (str, "false") == 0)
    *pb = 0;
  else
    return 1;

  return 0;
}

int
get_port (const char *port_str, int *pn)
{
  short port_num;
  long num;
  char *p;
  
  num = port_num = strtol (port_str, &p, 0);
  if (*p == 0)
    {
      if (num != port_num)
	{
	  mu_error ("bad port number: %s", port_str);
	  return 1;
	}
    }
  else
    {
      struct servent *sp = getservbyname (port_str, "tcp");
      if (!sp)
	{
	  mu_error ("unknown port name");
	  return 1;
	}
      port_num = ntohs (sp->s_port);
    }
  *pn = port_num;
  return 0;
}


/* Global handle for pop3.  */
mu_pop3_t pop3;

/* Flag if verbosity is needed.  */
int verbose;
#define VERBOSE_MASK(n) (1<<((n)+1))
#define SET_VERBOSE_MASK(n) (verbose |= VERBOSE_MASK (n))
#define CLR_VERBOSE_MASK(n) (verbose &= VERBOSE_MASK (n))
#define QRY_VERBOSE_MASK(n) (verbose & VERBOSE_MASK (n))
#define HAS_VERBOSE_MASK(n) (verbose & ~1)
#define SET_VERBOSE() (verbose |= 1)
#define CLR_VERBOSE() (verbose &= ~1)
#define QRY_VERBOSE() (verbose & 1)

enum pop_session_status
  {
    pop_session_disconnected,
    pop_session_connected,
    pop_session_logged_in
  };

enum pop_session_status pop_session_status;

int connect_argc;
char **connect_argv;

/* Host we are connected to. */
#define host connect_argv[0]
int port = 110;
char *username;

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
pop_prompt_vartab ()
{
  mu_vartab_t vtab;
  
  if (mu_vartab_create (&vtab))
    return;
  mu_vartab_define (vtab, "user",
		    (pop_session_status == pop_session_logged_in) ?
		      username : "not logged in", 1);
  mu_vartab_define (vtab, "host",
		    (pop_session_status != pop_session_disconnected) ?
		      host : "not connected", 1);
  mu_vartab_define (vtab, "program-name", mu_program_name, 1);
  mu_vartab_define (vtab, "canonical-program-name", "mu", 1);
  mu_vartab_define (vtab, "package", PACKAGE, 1);
  mu_vartab_define (vtab, "version", PACKAGE_VERSION, 1);
  mu_vartab_define (vtab, "status", pop_session_str (pop_session_status), 1);
  
  mu_vartab_destroy (&mutool_prompt_vartab);
  mutool_prompt_vartab = vtab;
}


static int
string_to_xlev (const char *name, int *pv)
{
  if (strcmp (name, "secure") == 0)
    *pv = MU_XSCRIPT_SECURE;
  else if (strcmp (name, "payload") == 0)
    *pv = MU_XSCRIPT_PAYLOAD;
  else
    return 1;
  return 0;
}

static int
change_verbose_mask (int set, int argc, char **argv)
{
  int i;
  
  for (i = 0; i < argc; i++)
    {
      int lev;
      
      if (string_to_xlev (argv[i], &lev))
	{
	  mu_error ("unknown level: %s", argv[i]);
	  return 1;
	}
      if (set)
	SET_VERBOSE_MASK (lev);
      else
	CLR_VERBOSE_MASK (lev);
    }
  return 0;
}

void
set_verbose (mu_pop3_t p)
{
  if (p)
    {
      if (QRY_VERBOSE ())
	{
	  mu_pop3_trace (p, MU_POP3_TRACE_SET);
	}
      else
	mu_pop3_trace (p, MU_POP3_TRACE_CLR);
    }
}

void
set_verbose_mask (mu_pop3_t p)
{
  if (p)
    {
      mu_pop3_trace_mask (p, QRY_VERBOSE_MASK (MU_XSCRIPT_SECURE)
			          ? MU_POP3_TRACE_SET : MU_POP3_TRACE_CLR,
			      MU_XSCRIPT_SECURE);
      mu_pop3_trace_mask (p, QRY_VERBOSE_MASK (MU_XSCRIPT_PAYLOAD)
			          ? MU_POP3_TRACE_SET : MU_POP3_TRACE_CLR,
			      MU_XSCRIPT_PAYLOAD);
    }
}

static int
com_verbose (int argc, char **argv)
{
  if (argc == 1)
    {
      if (QRY_VERBOSE ())
	{
	  mu_stream_printf (mustrout, "verbose is on");
	  if (HAS_VERBOSE_MASK ())
	    {
	      char *delim = " (";
	    
	      if (QRY_VERBOSE_MASK (MU_XSCRIPT_SECURE))
		{
		  mu_stream_printf (mustrout, "%ssecure", delim);
		  delim = ", ";
		}
	      if (QRY_VERBOSE_MASK (MU_XSCRIPT_PAYLOAD))
		mu_stream_printf (mustrout, "%spayload", delim);
	      mu_stream_printf (mustrout, ")");
	    }
	  mu_stream_printf (mustrout, "\n");
	}
      else
	mu_stream_printf (mustrout, "verbose is off\n");
    }
  else
    {
      int bv;

      if (get_bool (argv[1], &bv) == 0)
	{
	  verbose |= bv;
	  if (argc > 2)
	    change_verbose_mask (verbose, argc - 2, argv + 2);
	  set_verbose (pop3);
	}
      else if (strcmp (argv[1], "mask") == 0)
	change_verbose_mask (1, argc - 2, argv + 2);
      else if (strcmp (argv[1], "unmask") == 0)
	change_verbose_mask (0, argc - 2, argv + 2);
      else
	mu_error ("unknown subcommand");
      set_verbose_mask (pop3);
    }

  return 0;
}

static int
com_user (int argc, char **argv)
{
  int status;
  
  status = mu_pop3_user (pop3, argv[1]);
  if (status == 0)
    {
      username = strdup (argv[1]);
      pop_prompt_vartab ();
    }
  return status;
}

static int
com_apop (int argc, char **argv)
{
  int status;

  status = mu_pop3_apop (pop3, argv[1], argv[2]);
  if (status == 0)
    {
      username = strdup (argv[1]);
      pop_session_status = pop_session_logged_in;
    }
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
		mu_stream_printf (mustrout, "%s: %s\n", argv[i], elt);
	      else
		mu_stream_printf (mustrout, "%s is set\n", argv[i]);
	      break;

	    case MU_ERR_NOENT:
	      mu_stream_printf (mustrout, "%s is not set\n", argv[i]);
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
	      mu_stream_printf (mustrout, "CAPA: %s\n", capa ? capa : "");
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
	mu_stream_printf (mustrout, "Msg: %d UIDL: %s\n", msgno, uidl ? uidl : "");
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
	mu_stream_printf (mustrout, "Msg: %u Size: %lu\n", msgno, (unsigned long) size);
    }
  return status;
}

static int
com_noop (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  return mu_pop3_noop (pop3);
}

static void
echo_off (struct termios *stored_settings)
{
  struct termios new_settings;
  tcgetattr (0, stored_settings);
  new_settings = *stored_settings;
  new_settings.c_lflag &= (~ECHO);
  tcsetattr (0, TCSANOW, &new_settings);
}

static void
echo_on (struct termios *stored_settings)
{
  tcsetattr (0, TCSANOW, stored_settings);
}

static int
com_pass (int argc, char **argv)
{
  int status;
  char pass[256];
  char *pwd;
  
  if (argc == 1)
    {
      struct termios stored_settings;

      printf ("passwd:");
      fflush (stdout);
      echo_off (&stored_settings);
      fgets (pass, sizeof pass, stdin);
      echo_on (&stored_settings);
      putchar ('\n');
      fflush (stdout);
      pass[strlen (pass) - 1] = '\0';	/* nuke the trailing line.  */
      pwd = pass;
    }
  else
    pwd = argv[1];
  status = mu_pop3_pass (pop3, pwd);
  if (status == 0)
    {
      pop_session_status = pop_session_logged_in;
      pop_prompt_vartab ();
    }
  return status;
}

static int
com_stat (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  size_t count = 0;
  mu_off_t size = 0;
  int status = 0;

  status = mu_pop3_stat (pop3, &count, &size);
  mu_stream_printf (mustrout, "Mesgs: %lu Size %lu\n",
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
      pop_prompt_vartab ();
    }
  return 0;
}

static int
com_connect (int argc, char **argv)
{
  int status;
  int n = 0;
  int tls = 0;
  int i = 1;
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-tls") == 0)
	{
	  if (WITH_TLS)
	    tls = 1;
	  else
	    {
	      mu_error ("TLS not supported");
	      return 0;
	    }
	}
      else
	break;
    }

  argc -= i;
  argv += i;
  
  if (argc >= 2)
    {
      if (get_port (argv[1], &n))
	return 0;
    }
  else if (tls)
    n = MU_POP3_DEFAULT_SSL_PORT;
  else
    n = MU_POP3_DEFAULT_PORT;
  
  if (pop_session_status != pop_session_disconnected)
    com_disconnect (0, NULL);
  
  status = mu_pop3_create (&pop3);
  if (status == 0)
    {
      mu_stream_t tcp;

      if (QRY_VERBOSE ())
	{
	  set_verbose (pop3);
	  set_verbose_mask (pop3);
	}
      status = mu_tcp_stream_create (&tcp, argv[0], n, MU_STREAM_READ);
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
      connect_argv = xcalloc (argc, sizeof (*connect_argv));
      for (i = 0; i < argc; i++)
	connect_argv[i] = xstrdup (argv[i]);
      connect_argv[i] = NULL;
      port = n;
      pop_session_status = pop_session_connected;

      pop_prompt_vartab ();
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
	  mu_stream_printf (mustrout, "Try 'exit' to leave %s\n", mu_program_name);
	}
    }
  else
    mu_stream_printf (mustrout, "Try 'exit' to leave %s\n", mu_program_name);
  return status;
}

struct mutool_command pop_comtab[] = {
  { "apop",       3,  3, com_apop,
    "Authenticate with APOP: APOP user secret" },
  { "capa",       1, -1, com_capa,
    "List capabilities: capa [-reread] [names...]" },
  { "disconnect", 1, 1,
    com_disconnect, "Close connection: disconnect" },
  { "dele",       2, 2, com_dele,
    "Mark message: DELE msgno" },
  { "list",       1, 2, com_list,
    "List messages: LIST [msgno]" },
  { "noop",       1, 1, com_noop,
    "Send no operation: NOOP" },
  { "pass",       1, 2, com_pass,
    "Send passwd: PASS [passwd]" },
  { "connect",    1, 4, com_connect,
    "Open connection: connect [-tls] hostname port" },
  { "quit",       1, 1, com_quit,
    "Go to Update state : QUIT" },
  { "retr",       2, 2, com_retr,
    "Dowload message: RETR msgno" },
  { "rset",       1, 1, com_rset,
    "Unmark all messages: RSET" },
  { "stat",       1, 1, com_stat,
    "Get the size and count of mailbox : STAT" },
  { "stls",       1, 1, com_stls,
    "Start TLS negotiation" },
  { "top",        2, 3, com_top,
    "Get the header of message: TOP msgno [lines]" },
  { "uidl",       1, 2, com_uidl,
    "Get the unique id of message: UIDL [msgno]" },
  { "user",       2, 2, com_user,
    "send login: USER user" },
  { "verbose",    1, 4, com_verbose,
    "Enable Protocol tracing: verbose [on|off|mask|unmask] [x1 [x2]]" },
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
  mutool_shell_prompt = xstrdup ("pop> ");
  pop_prompt_vartab ();
  mutool_shell ("pop", pop_comtab);
  return 0;
}

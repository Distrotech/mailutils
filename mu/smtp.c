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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mailutils/cctype.h>
#include <mailutils/mailutils.h>
#include <mailutils/smtp.h>
#include <argp.h>
#include "mu.h"

static char smtp_doc[] = N_("mu smtp - run a SMTP session.");
char smtp_docstring[] = N_("run a SMTP session");
static char smtp_args_doc[] = "";

static struct argp_option smtp_options[] = {
  { NULL }
};

static error_t
smtp_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp smtp_argp = {
  smtp_options,
  smtp_parse_opt,
  smtp_args_doc,
  smtp_doc,
  NULL,
  NULL,
  NULL
};

enum smtp_session_status
  {
    smtp_session_disconnected,
    smtp_session_connected,
    smtp_session_logged_in
  };

static enum smtp_session_status smtp_session_status;
static int connect_argc;
static char **connect_argv;

/* Host we are connected to. */
#define host connect_argv[0]
static int port = 25;

static char *sender;
static mu_list_t recipients;

static char *msgfile;
static int temp_msgfile;
static mu_smtp_t smtp;

const char *
smtp_session_str (enum smtp_session_status stat)
{
  switch (stat)
    {
    case smtp_session_disconnected:
      return "disconnected";
      
    case smtp_session_connected:
      return "connected";
      
    case smtp_session_logged_in:
      return "logged in";
    }
  return "unknown";
}

static void
smtp_prompt_env ()
{
  const char *value;
  
  if (!mutool_prompt_env)
    mutool_prompt_env = mu_calloc (2*7 + 1, sizeof(mutool_prompt_env[0]));

  mutool_prompt_env[0] = "user";
  mutool_prompt_env[1] = "[nouser]";
  if (smtp_session_status == smtp_session_logged_in &&
      mu_smtp_get_param (smtp, MU_SMTP_PARAM_USERNAME, &value) == 0)
    mutool_prompt_env[1] = (char*) value;

  mutool_prompt_env[2] = "host"; 
  mutool_prompt_env[3] = (smtp_session_status != smtp_session_disconnected) ?
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
  mutool_prompt_env[13] = (char*) smtp_session_str (smtp_session_status);

  mutool_prompt_env[14] = NULL;
}

static void
smtp_set_verbose (void)
{
  if (smtp)
    {
      if (QRY_VERBOSE ())
	mu_smtp_trace (smtp, MU_SMTP_TRACE_SET);
      else
	mu_smtp_trace (smtp, MU_SMTP_TRACE_CLR);
    }
}

static void
smtp_set_verbose_mask (void)
{
  if (smtp)
    {
      mu_smtp_trace_mask (smtp, QRY_VERBOSE_MASK (MU_XSCRIPT_SECURE)
			          ? MU_SMTP_TRACE_SET : MU_SMTP_TRACE_CLR,
			      MU_XSCRIPT_SECURE);
      mu_smtp_trace_mask (smtp, QRY_VERBOSE_MASK (MU_XSCRIPT_PAYLOAD)
			          ? MU_SMTP_TRACE_SET : MU_SMTP_TRACE_CLR,
			      MU_XSCRIPT_PAYLOAD);
    }
}

static int
com_verbose (int argc, char **argv)
{
  return shell_verbose (argc, argv,
			smtp_set_verbose, smtp_set_verbose_mask);
}

static int
smtp_error_handler (int rc)
{
  if (rc == 0 || rc == MU_ERR_REPLY)
    {
      char code[4];
      const char *repl;
      
      mu_smtp_replcode (smtp, code);
      mu_smtp_sget_reply (smtp, &repl);
      mu_printf ("%s %s\n", code, repl);
    }
  return rc;
}

static int
com_disconnect (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  if (smtp)
    {
      mu_smtp_disconnect (smtp);
      mu_smtp_destroy (&smtp);
      smtp = NULL;
      
      mu_argcv_free (connect_argc, connect_argv);
      connect_argc = 0;
      connect_argv = NULL;
      smtp_session_status = smtp_session_disconnected;
      smtp_prompt_env ();
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
  
  if (smtp_session_status != smtp_session_disconnected)
    com_disconnect (0, NULL);
  
  status = mu_smtp_create (&smtp);
  if (status == 0)
    {
      mu_stream_t tcp;
      struct mu_sockaddr *sa;
      struct mu_sockaddr_hints hints;

      if (QRY_VERBOSE ())
	{
	  smtp_set_verbose ();
	  smtp_set_verbose_mask ();
	}

      memset (&hints, 0, sizeof (hints));
      hints.flags = MU_AH_DETECT_FAMILY;
      hints.port = tls ? 465 : 25;
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
	  mu_smtp_set_carrier (smtp, tcp);
	  status = smtp_error_handler (mu_smtp_open (smtp));
	}
      else
	{
	  mu_smtp_destroy (&smtp);
	  smtp = NULL;
	}
    }

  if (status)
    mu_error ("Failed to create smtp: %s", mu_strerror (status));
  else
    {
      connect_argc = argc;
      connect_argv = mu_calloc (argc, sizeof (*connect_argv));
      for (i = 0; i < argc; i++)
	connect_argv[i] = mu_strdup (argv[i]);
      connect_argv[i] = NULL;
      port = n;
      smtp_session_status = smtp_session_connected;

      smtp_prompt_env ();
    }

  /* Provide a default URL.  Authentication functions require it, see comment
     in smtp_auth.c:119. */
  mu_smtp_set_param (smtp, MU_SMTP_PARAM_URL, "smtp://");
  
  return status;
}

static int
com_capa (int argc, char **argv)
{
  mu_iterator_t iterator = NULL;
  int status = 0;
  int i = 1;
  
  if (i < argc)
    {
      for (; i < argc; i++)
	{
	  const char *elt;
	  int rc = mu_smtp_capa_test (smtp, argv[i], &elt);
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
	      return smtp_error_handler (rc);
	    }
	}
    }
  else
    {
      status = mu_smtp_capa_iterator (smtp, &iterator);

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
com_ehlo (int argc, char **argv)
{
  if (argc == 1)
    {
      if (mu_smtp_test_param (smtp, MU_SMTP_PARAM_DOMAIN))
	{
	  mu_error (_("no domain set"));
	  return 0;
	}
    }
  else
    mu_smtp_set_param (smtp, MU_SMTP_PARAM_DOMAIN, argv[1]);
  return com_capa (1, argv);
}

static int
com_rset (int argc, char **argv)
{
  return smtp_error_handler (mu_smtp_rset (smtp));
}

static int
com_quit (int argc, char **argv)
{
  int status = 0;
  if (smtp)
    {
      if (smtp_error_handler (mu_smtp_quit (smtp)) == 0)
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
com_from (int argc, char **argv)
{
  if (argc == 1)
    {
      if (!sender)
	{
	  mu_error (_("no sender address"));
	  return 0;
	}
    }
  else
    {
      free (sender);
      sender = mu_strdup (argv[1]);
    }
  return smtp_error_handler (mu_smtp_mail_basic (smtp, sender, NULL)); 
}

static int
send_rcpt_to (void *item, void *data)
{
  return smtp_error_handler (mu_smtp_rcpt_basic (smtp, (char*) item, NULL));
}

static int
com_to (int argc, char **argv)
{
  int rc;

  if (argc == 1)
    {
      if (mu_list_is_empty (recipients))
	{
	  mu_error (_("no recipients"));
	  return 1;
	}
      mu_list_foreach (recipients, send_rcpt_to, NULL);
      rc = 0;
    }
  else
    {
      if (!recipients)
	mu_list_create (&recipients);
      mu_list_set_destroy_item (recipients, mu_list_free_item);
      rc = smtp_error_handler (mu_smtp_rcpt_basic (smtp, argv[1], NULL));
      if (rc == 0)
	mu_list_append (recipients, mu_strdup (argv[1]));
    }
  return rc;
}

static int
edit (const char *file)
{
  char *ed;
  char *edv[3];
  int rc, status;
  
  ed = getenv ("VISUAL");
  if (!ed)
    {
      ed = getenv ("EDITOR");
      if (!ed)
	ed = "/bin/ed";
    }
  
  edv[0] = ed;
  edv[1] = (char*) file;
  edv[2] = NULL;

  rc = mu_spawnvp (edv[0], edv, &status);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_spawnvp", edv[0], rc);
  return rc;
}

struct rcptout
{
  mu_stream_t str;
  int n;
};

static int
print_rcpt (void *item, void *data)
{
  struct rcptout *p = data;
  if (p->n++)
    mu_stream_write (p->str, ", ", 2, NULL);
  mu_stream_printf (p->str, "%s", (char *)item);
  return 0;
}
    
static int
edit_file (const char *fname, int inplace)
{
  int rc;
  
  if (fname && !inplace)
    {
      mu_stream_t istr, ostr;
      
      rc = mu_file_stream_create (&istr, fname, MU_STREAM_READ|MU_STREAM_SEEK);
      if (rc == 0)
	{
	  char *tempfile = mu_tempname (NULL);
	  rc = mu_file_stream_create (&ostr, tempfile,
				      MU_STREAM_CREAT|MU_STREAM_WRITE);
	  if (rc)
	    {
	      free (tempfile);
	      mu_error (_("cannot create temporary file: %s"),
			mu_strerror (rc));
	      return -1;
	    }
	  rc = mu_stream_copy (ostr, istr, 0, NULL);
	  if (rc)
	    {
	      unlink (tempfile);	
	      free (tempfile);
	      mu_error (_("error copying to temporary file: %s"),
			mu_strerror (rc));
	      return -1;
	    }
	  mu_stream_unref (ostr);
	  free (msgfile);
	  msgfile = tempfile;
	  temp_msgfile = 1;
	}
      else if (rc != ENOENT)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create", fname, rc);
	  return 1;
	}
      mu_stream_unref (istr);
    }
  else if (!fname)
    {
      struct rcptout rcptout;
      
      if (temp_msgfile)
	unlink (msgfile);
      free (msgfile);
      msgfile = mu_tempname (NULL);
      temp_msgfile = 1;

      rc = mu_file_stream_create (&rcptout.str, msgfile,
				  MU_STREAM_CREAT|MU_STREAM_WRITE);
      if (rc)
	{
	  mu_error (_("cannot open temporary file for writing: %s"),
		    mu_strerror (rc));
	  return 1;
	}
      rcptout.n = 0;
      if (sender)
	mu_stream_printf (rcptout.str, "From: %s\n", sender);
      else
	mu_stream_printf (rcptout.str, "From: \n");
      mu_stream_printf (rcptout.str, "To: ");
      mu_list_foreach (recipients, print_rcpt, &rcptout);
      mu_stream_write (rcptout.str, "\n", 1, NULL);
      mu_stream_printf (rcptout.str, "Subject: \n\n");
      mu_stream_unref (rcptout.str);
    }
  else
    {
      free (msgfile);
      msgfile = mu_strdup (fname);
      temp_msgfile = 0;
    }
	  
  do
    {
      if (edit (msgfile))
	return 1;
    }
  while ((rc = mu_getans ("seqSEQ", _("What now: [s]end, [e]dit, [q]uit")))
	 == 'e' || rc == 'E');
  
  return rc == 'q' || rc == 'Q';
}

static int
com_send (int argc, char **argv)
{
  int rc;
  mu_stream_t instr;
  
  if (argc == 1)
    {
      if (msgfile)
	{
	  switch (mu_getans ("rReEdD",
			     _("Previous message exists. "
			       "What now: [r]euse, [e]dit, "
			       "[u]se as a template or\n"
			       "[d]rop and start from scratch")))
	    {
	    case 'r':
	    case 'R':
	      rc = 0;
	      break;

	    case 'e':
	    case 'E':
	      rc = edit_file (msgfile, 1);
	      break;

	    case 'd':
	    case 'D':
	      if (temp_msgfile)
		unlink (msgfile);
	      free (msgfile);
	      msgfile = NULL;
	      temp_msgfile = 0;
	      rc = edit_file (NULL, 0);
	      break;

	    case 'u':
	    case 'U':
	      rc = edit_file (msgfile, 0);
	    }
	}
      else
	rc = edit_file (NULL, 0);
      if (rc)
	return 0;
    }
  else
    {
      if (temp_msgfile)
	unlink (msgfile);
      free (msgfile);
      msgfile = NULL;
      temp_msgfile = 0;
      msgfile = mu_strdup (argv[1]);
    }
  
  rc = mu_file_stream_create (&instr, msgfile, MU_STREAM_READ|MU_STREAM_SEEK);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create", msgfile, rc);
      return 1;
    }

  rc = mu_smtp_send_stream (smtp, instr);
  mu_stream_unref (instr);
  
  if (rc)
    smtp_error_handler (rc);
  else
    rc = smtp_error_handler (mu_smtp_dot (smtp));
  
  return rc;
}

static int
com_starttls (int argc, char **argv)
{
  if (mu_smtp_capa_test (smtp, "STARTTLS", NULL) == 0)
    return smtp_error_handler (mu_smtp_starttls (smtp));
  else
    mu_error (_("remote party does not offer STARTTLS"));
  return 1;
}

static int
com_auth (int argc, char **argv)
{
  int rc, i;

  rc = mu_smtp_clear_auth_mech (smtp);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_clear_auth_mech", NULL, rc);
      return MU_ERR_FAILURE;
    }
  for (i = 1; i < argc; i++)
    if ((rc = mu_smtp_add_auth_mech (smtp, argv[1])))
      {
	mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_add_auth_mech", NULL, rc);
	return MU_ERR_FAILURE;
      }

  rc = mu_smtp_auth (smtp);
	
  switch (rc)
    {
    case 0:
      smtp_session_status = smtp_session_logged_in;
      break;
      
    case ENOSYS:
      mu_error (_("authentication not implemented"));
      break;
      
    case MU_ERR_NOENT:
      mu_error (_("no suitable authentication mechanism found"));
      break;

    default:
      smtp_error_handler (rc);
      return rc;
    }
  return 0;
}

static struct mu_kwd paramtab[] = {
  { "domain",      MU_SMTP_PARAM_DOMAIN },   
  { "username",    MU_SMTP_PARAM_USERNAME }, 
  { "password",    MU_SMTP_PARAM_PASSWORD }, 
  { "service",     MU_SMTP_PARAM_SERVICE },
  { "realm",       MU_SMTP_PARAM_REALM },    
  { "host",        MU_SMTP_PARAM_HOST },
  { "url",         MU_SMTP_PARAM_URL },
  { NULL }
};

static int
get_param (int param, char *prompt, char **retval)
{
  int rc;
  
  if (param == MU_SMTP_PARAM_PASSWORD)
    {
      rc = mu_getpass (mu_strin, mu_strout, prompt, retval);
      if (rc)
	mu_diag_funcall (MU_DIAG_ERROR, "mu_getpass", NULL, rc);
    }
  else
    {
      char *buf = NULL;
      size_t size = 0;
      rc = mu_stream_write (mu_strout, prompt, strlen (prompt), NULL);
      if (rc)
	return rc;
      mu_stream_flush (mu_strout);
      rc = mu_stream_getline (mu_strin, &buf, &size, NULL);
      if (rc == 0)
	{
	  mu_rtrim_cset (buf, "\n");
	  *retval  = buf;
	}
    }
  return rc;
}

static int
com_set (int argc, char **argv)
{
  int param, i, rc;
  
  for (i = 1; i < argc; i += 2)
    {
      if (mu_kwd_xlat_name (paramtab, argv[i], &param))
	{
	  mu_error (_("unrecognized parameter: %s"), argv[i]);
	  continue;
	}
      if (i + 1 < argc)
	{
	  rc = mu_smtp_set_param (smtp, param, argv[i+1]);
	  if (rc)
	    mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_set_param", argv[i], rc);
	}
      else
	{
	  char *prompt, *value;
	  mu_asprintf (&prompt, "%s: ", argv[i]);
	  rc = get_param (param, prompt, &value);
	  free (prompt);
	  if (rc)
	    mu_error (_("error reading value: %s"), mu_strerror (rc));
	  else
	    {
	      rc = mu_smtp_set_param (smtp, param, value);
	      if (param == MU_SMTP_PARAM_PASSWORD)
		memset (value, 0, strlen (value));
	      if (rc)
		mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_set_param", argv[i],
				 rc);
	      free (value);
	    }
	}
    }
  return 0;
}

static int
com_clear (int argc, char **argv)
{
  int param, i, rc;

  if (argc > 1)
    {
      for (i = 1; i < argc; i++)
	{
	  if (mu_kwd_xlat_name (paramtab, argv[i], &param))
	    {
	      mu_error (_("unrecognized parameter: %s"), argv[i]);
	      continue;
	    }
	  rc = mu_smtp_set_param (smtp, param, NULL);
	  if (rc)
	    mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_set_param", argv[i], rc);
	}
    }
  else
    {
      for (i = 0; paramtab[i].name; i++)
	{
	  rc = mu_smtp_set_param (smtp, paramtab[i].tok, NULL);
	  if (rc)
	    mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_set_param",
			     paramtab[i].name, rc);
	}
    }
  return 0;
}

static int
com_list_param (int argc, char **argv)
{
  int param, i, rc;
  const char *value;
  
  if (argc > 1)
    {
      for (i = 1; i < argc; i++)
	{
	  if (mu_kwd_xlat_name (paramtab, argv[i], &param))
	    {
	      mu_error (_("unrecognized parameter: %s"), argv[i]);
	      continue;
	    }
	  rc = mu_smtp_get_param (smtp, param, &value);
	  if (rc)
	    mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_set_param", argv[i], rc);
	  else if (value)
	    mu_printf ("%s = %s\n", argv[i], value);
	  else
	    mu_printf (_("%s not set\n"), argv[i]);
	}
    }
  else
    {
      for (i = 0; paramtab[i].name; i++)
	{
	  rc = mu_smtp_get_param (smtp, paramtab[i].tok, &value);
	  if (rc)
	    mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_set_param",
			     paramtab[i].name, rc);
	  else if (value)
	    mu_printf ("%s = %s\n", paramtab[i].name, value);
	  else
	    mu_printf (_("%s not set\n"), paramtab[i].name);
	}
    }
  return 0;
}

static int
com_smtp_command (int argc, char **argv)
{
  int rc;
  mu_iterator_t itr;
  
  rc = mu_smtp_cmd (smtp, argc - 1, argv + 1);
  smtp_error_handler (rc);
  if (rc)
    return rc;
  rc = mu_smtp_get_reply_iterator (smtp, &itr);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_smtp_get_reply_iterator", NULL, rc);
      return 1;
    }

  for (mu_iterator_first (itr);
       !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *str = NULL;
      mu_iterator_current (itr, (void **) &str);
      mu_printf ("%s\n", str);
    }
  mu_iterator_destroy (&itr);
  return 0;
}

struct mutool_command smtp_comtab[] = {
  { "connect",    1, 4, 0, com_connect,
    /* TRANSLATORS: -tls is a keyword. */
    N_("[-tls] HOSTNAME [PORT]"),
    N_("open connection") },

  { "set",        2, -1, 0, com_set,
    N_("PARAM [ARG...]"),
    N_("Set connection parameter") },
  { "clear",      1, -1, 0, com_clear,
    N_("[PARAM...]"),
    N_("Clear connection parameters") },
  
  { "list",       1, -1, 0, com_list_param,
    N_("[PARAM...]"),
    N_("List connection parameters") },
    
  { "auth",       2, -1, 0, com_auth,
    N_("MECH [MECH...]"),
    N_("Authenticate") },
  
  { "ehlo",       1, 2, 0, com_ehlo,
    N_("[DOMAIN]"),
    N_("Greet the server") },

  { "capa",       1, -1, 0, com_capa,
    N_("[NAME...]"),
    N_("list server capabilities") },

  { "starttls",   1, 1, 0, com_starttls,
    NULL,
    N_("initiate encrypted connection") },
  
  { "rset",       1, 1, 0, com_rset,
    NULL,
    N_("reset the session state") },

  { "from",       1, 2, 0, com_from,
    N_("[EMAIL]"),
    N_("set sender email") },
  
  { "to",       1, 2, 0, com_to,
    N_("[EMAIL]"),
    N_("set recipient email") },

  { "send",     1, 2, 0, com_send,
    N_("[FILE]"),
    N_("send message") },

  { "smtp",     2, -1, 0, com_smtp_command,
    N_("COMMAND [ARGS...]"),
    N_("send an arbitrary COMMAND") },

  { "quit",       1, 1, 0, com_quit,
    NULL,
    N_("quit the session") },
  
  { "verbose",    1, 4, 0, com_verbose,
    "[on|off|mask|unmask] [secure [payload]]",
    N_("control the protocol tracing") },

  { NULL }
};  

int
mutool_smtp (int argc, char **argv)
{
  int index;
  
  mu_registrar_record (mu_smtp_record);
  mu_registrar_record (mu_smtps_record);
  
  if (argp_parse (&smtp_argp, argc, argv, 0, &index, NULL))
    return 1;

  argc -= index;

  if (argc)
    {
      mu_error (_("bad arguments"));
      return 1;
    }
  
  mutool_shell_prompt = mu_strdup ("smtp> ");
  smtp_prompt_env ();
  mutool_shell ("smtp", smtp_comtab);

  if (temp_msgfile)
    unlink (msgfile);

  return 0;
}

/*
  MU Setup: smtp
  mu-handler: mutool_smtp
  mu-docstring: smtp_docstring
  End MU Setup:
*/

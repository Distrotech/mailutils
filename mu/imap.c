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
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mailutils/mailutils.h>
#include <mailutils/imap.h>
#include <mailutils/imapio.h>
#include <mailutils/imaputil.h>
#include <mailutils/msgset.h>
#include "mu.h"
#include "argp.h"

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
static int uid_mode;

static enum mu_imap_session_state
current_imap_state ()
{
  int state;
  if (imap == NULL)
    state = MU_IMAP_SESSION_INIT;
  else
    state = mu_imap_session_state (imap);
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

static mu_msgset_t
parse_msgset (const char *arg)
{
  int status;
  mu_msgset_t msgset;
  char *p;
  
  status = mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_create", NULL, status);
      return NULL;
    }
  status = mu_msgset_parse_imap (msgset, MU_MSGSET_NUM, arg, &p);
  if (status)
    {
      mu_error (_("failed to parse message set near \"%s\": %s"),
		p, mu_strerror (status));
      mu_msgset_destroy (&msgset);
    }
  return msgset;
}

static void
report_failure (const char *what, int status)
{
  const char *str;
      
  mu_error (_("%s failed: %s"), what, mu_strerror (status));
  if (mu_imap_strerror (imap, &str) == 0)
    mu_error (_("server reply: %s"), str);
}

static int connect_argc;
static char **connect_argv;
#define host connect_argv[0]

static char *username;

static void
imap_prompt_env ()
{
  enum mu_imap_session_state state = current_imap_state ();
  if (!mutool_prompt_env)
    mutool_prompt_env = mu_calloc (2*7 + 1, sizeof(mutool_prompt_env[0]));

  mutool_prompt_env[0] = "user";
  mutool_prompt_env[1] = (state >= MU_IMAP_SESSION_AUTH && username) ?
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
  if (mu_imap_session_state_str (state,
				 (const char **) &mutool_prompt_env[13]))
    mutool_prompt_env[12] = NULL;

  mutool_prompt_env[14] = NULL;
}

/* Callbacks */
static void
imap_preauth_callback (void *data, int code, size_t sdat, void *pdat)
{
  const char *text = pdat;
  if (text)
    mu_diag_output (MU_DIAG_INFO, _("session authenticated: %s"), text);
  else
    mu_diag_output (MU_DIAG_INFO, _("session authenticated"));
}

static void
imap_bye_callback (void *data, int code, size_t sdat, void *pdat)
{
  const char *text = pdat;
  if (text)
    mu_diag_output (MU_DIAG_INFO, _("server is closing connection: %s"), text);
  else
    mu_diag_output (MU_DIAG_INFO, _("server is closing connection"));
}

static void
imap_bad_callback (void *data, int code, size_t sdat, void *pdat)
{
  const char *text = pdat;
  mu_diag_output (MU_DIAG_CRIT, "SERVER ALERT: %s", text);
}

/* Fetch callback */
static void
format_email (mu_stream_t str, const char *name, mu_address_t addr)
{
  mu_stream_printf (str, "  %s = ", name);
  if (!addr)
    mu_stream_printf (str, "NIL");
  else
    mu_stream_format_address (str, addr);
  mu_stream_printf (str, "\n");
}

static void
format_date (mu_stream_t str, char *name,
	     struct tm *tm, struct mu_timezone *tz)
{
  char date[128];
  
  strftime (date, sizeof (date), "%a %b %e %H:%M:%S", tm);
  mu_stream_printf (str, "  %s = %s", name, date);
  if (tz->tz_name)
    mu_stream_printf (str, " %s", tz->tz_name);
  else
    {
      int off = tz->utc_offset;
      if (off < 0)
	{
	  mu_stream_printf (str, " -");
	  off = - off;
	}
      else
	mu_stream_printf (str, " +");
      off /= 60;
      mu_stream_printf (str, "%02d%02d", off / 60, off % 60);
    }
  mu_stream_printf (str, "\n");
}

#define S(str) ((str) ? (str) : "")

static void
print_param (mu_stream_t ostr, const char *prefix, mu_assoc_t assoc,
	     int indent)
{
  mu_iterator_t itr;
  int i;

  mu_stream_printf (ostr, "%*s%s:\n", indent, "", prefix);
  indent += 4;
  if (mu_assoc_get_iterator (assoc, &itr))
    return;
  for (i = 0, mu_iterator_first (itr);
       !mu_iterator_is_done (itr);
       i++, mu_iterator_next (itr))
    {
      const char *name;
      struct mu_mime_param *p;
      
      mu_iterator_current_kv (itr, (const void **)&name, (void**)&p);
      mu_stream_printf (ostr, "%*s%d: %s=%s\n", indent, "", i, name, p->value);
    }
  mu_iterator_destroy (&itr);
}

struct print_data
{
  mu_stream_t ostr;
  int num;
  int level;
};

static void print_bs (mu_stream_t ostr,
		      struct mu_bodystructure *bs, int level);

static int
print_item (void *item, void *data)
{
  struct mu_bodystructure *bs = item;
  struct print_data *pd = data;
  mu_stream_printf (pd->ostr, "%*sPart #%d\n", (pd->level-1) << 2, "",
		    pd->num);
  print_bs (pd->ostr, bs, pd->level);
  ++pd->num;
  return 0;
}

static void
print_address (mu_stream_t ostr, const char *title, mu_address_t addr,
	       int indent)
{
  mu_stream_printf (ostr, "%*s%s: ", indent, "", title);
  mu_stream_format_address (mu_strout, addr);
  mu_stream_printf (ostr, "\n");
}

static void
print_imapenvelope (mu_stream_t ostr, struct mu_imapenvelope *env, int level)
{
  int indent = (level << 2);

  mu_stream_printf (ostr, "%*sEnvelope:\n", indent, "");
  indent += 4;
  mu_stream_printf (ostr, "%*sTime: ", indent, "");
  mu_c_streamftime (mu_strout, "%c%n", &env->date, &env->tz);
  mu_stream_printf (ostr, "%*sSubject: %s\n", indent, "", S(env->subject));
  print_address (ostr, "From", env->from, indent);
  print_address (ostr, "Sender", env->sender, indent);
  print_address (ostr, "Reply-to", env->reply_to, indent);
  print_address (ostr, "To", env->to, indent);
  print_address (ostr, "Cc", env->cc, indent);
  print_address (ostr, "Bcc", env->bcc, indent);
  mu_stream_printf (ostr, "%*sIn-Reply-To: %s\n", indent, "",
		    S(env->in_reply_to));
  mu_stream_printf (ostr, "%*sMessage-ID: %s\n", indent, "",
		    S(env->message_id));
}

static void
print_bs (mu_stream_t ostr, struct mu_bodystructure *bs, int level)
{
  int indent = level << 2;
  mu_stream_printf (ostr, "%*sbody_type=%s\n", indent, "", S(bs->body_type));
  mu_stream_printf (ostr, "%*sbody_subtype=%s\n", indent, "",
		    S(bs->body_subtype));
  print_param (ostr, "Parameters", bs->body_param, indent);
  mu_stream_printf (ostr, "%*sbody_id=%s\n", indent, "", S(bs->body_id));
  mu_stream_printf (ostr, "%*sbody_descr=%s\n", indent, "", S(bs->body_descr));
  mu_stream_printf (ostr, "%*sbody_encoding=%s\n", indent, "",
		    S(bs->body_encoding));
  mu_stream_printf (ostr, "%*sbody_size=%lu\n", indent, "",
		    (unsigned long) bs->body_size);
  /* Optional */
  mu_stream_printf (ostr, "%*sbody_md5=%s\n", indent, "", S(bs->body_md5));
  mu_stream_printf (ostr, "%*sbody_disposition=%s\n", indent, "",
		    S(bs->body_disposition));
  print_param (ostr, "Disposition Parameters", bs->body_disp_param, indent);
  mu_stream_printf (ostr, "%*sbody_language=%s\n", indent, "",
		    S(bs->body_language));
  mu_stream_printf (ostr, "%*sbody_location=%s\n", indent, "",
		    S(bs->body_location));

  mu_stream_printf (ostr, "%*sType ", indent, "");
  switch (bs->body_message_type)
    {
    case mu_message_other:
      mu_stream_printf (ostr, "mu_message_other\n");
      break;
      
    case mu_message_text:
      mu_stream_printf (ostr, "mu_message_text:\n%*sbody_lines=%lu\n",
			indent + 4, "",
			(unsigned long) bs->v.text.body_lines);
      break;
      
    case mu_message_rfc822:
      mu_stream_printf (ostr, "mu_message_rfc822:\n%*sbody_lines=%lu\n",
			indent + 4, "",
		 (unsigned long) bs->v.rfc822.body_lines);
      print_imapenvelope (ostr, bs->v.rfc822.body_env, level + 1);
      print_bs (ostr, bs->v.rfc822.body_struct, level + 1);
      break;
      
    case mu_message_multipart:
      {
	struct print_data pd;
	pd.ostr = ostr;
	pd.num = 0;
	pd.level = level + 1;
	mu_stream_printf (ostr, "mu_message_multipart:\n");
	mu_list_foreach (bs->v.multipart.body_parts, print_item, &pd);
      }
    }
}

static int
fetch_response_printer (void *item, void *data)
{
  union mu_imap_fetch_response *resp = item;
  mu_stream_t str = data;

  switch (resp->type)
    {
    case MU_IMAP_FETCH_BODY:
      mu_stream_printf (str, "BODY [");
      if (resp->body.partv)
	{
	  size_t i;
	  
	  for (i = 0; i < resp->body.partc; i++)
	    mu_stream_printf (str, "%lu.",
			      (unsigned long) resp->body.partv[i]);
	}
      if (resp->body.section)
	mu_stream_printf (str, "%s", resp->body.section);
      mu_stream_printf (str, "]");
      mu_stream_printf (str, "\nBEGIN\n%s\nEND\n", resp->body.text);
      break;
      
    case MU_IMAP_FETCH_BODYSTRUCTURE:
      /* FIXME */
      mu_stream_printf (str, "BODYSTRUCTURE:\nBEGIN\n");
      print_bs (str, resp->bodystructure.bs, 0);
      mu_stream_printf (str, "END\n");
      break;
      
    case MU_IMAP_FETCH_ENVELOPE:
      {
	struct mu_imapenvelope *env = resp->envelope.imapenvelope;
	
	mu_stream_printf (str, "ENVELOPE:\n");
	
	format_date (str, "date", &env->date, &env->tz);
	mu_stream_printf (str, "  subject = %s\n",
			  env->subject ?
			   env->subject : "NIL");

	format_email (str, "from", env->from);
	format_email (str, "sender", env->sender);
	format_email (str, "reply-to", env->reply_to);
	format_email (str, "to", env->to);
	format_email (str, "cc", env->cc);
	format_email (str, "bcc", env->bcc);

	mu_stream_printf (str, "  in-reply-to = %s\n",
			  env->in_reply_to ?
			   env->in_reply_to : "NIL");
	mu_stream_printf (str, "  message-id = %s\n",
			  env->message_id ?
			   env->message_id : "NIL");
      }
      break;
      
    case MU_IMAP_FETCH_FLAGS:
      mu_stream_printf (str, "  flags = ");
      mu_imap_format_flags (str, resp->flags.flags, 1);
      mu_stream_printf (str, "\n");
      break;
      
    case MU_IMAP_FETCH_INTERNALDATE:
      format_date (str, "internaldate", &resp->internaldate.tm,
		   &resp->internaldate.tz);
      break;
      
    case MU_IMAP_FETCH_RFC822_SIZE:
      mu_stream_printf (str, "  size = %lu\n",
			(unsigned long) resp->rfc822_size.size);
      break;
      
    case MU_IMAP_FETCH_UID:
      mu_stream_printf (str, "  UID = %lu\n",
			(unsigned long) resp->uid.uid);
    }
  return 0;
}

static void
imap_fetch_callback (void *data, int code, size_t sdat, void *pdat)
{
  mu_stream_t str = data;
  mu_list_t list = pdat;

  mu_stream_printf (str, "Message #%lu:\n", (unsigned long) sdat);
  mu_list_foreach (list, fetch_response_printer, str);
  mu_stream_printf (str, "\n\n");
}

static int
com_uid (int argc, char **argv)
{
  int bv;
  
  if (argc == 1)
    mu_printf ("%s\n", uid_mode ? _("UID is on") : _("UID is off"));
  else if (get_bool (argv[1], &bv) == 0)
    uid_mode = bv;
  else
    mu_error (_("invalid boolean value"));
  return 0;
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
  enum mu_imap_session_state state;
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-tls") == 0)
	{
#ifdef WITH_TLS
	  tls = 1;
#else
	  mu_error (_("TLS not supported"));
	  return 0;
#endif
	}
      else
	break;
    }

  argc -= i;
  argv += i;
  
  state = current_imap_state ();
  
  if (state != MU_IMAP_SESSION_INIT)
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
		  mu_error (_("cannot create TLS stream: %s"),
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
					      imap_preauth_callback,
					      NULL);
	  mu_imap_register_callback_function (imap, MU_IMAP_CB_BYE,
					      imap_bye_callback,
					      NULL);
	  mu_imap_register_callback_function (imap, MU_IMAP_CB_BAD,
					      imap_bad_callback,
					      NULL);
	  mu_imap_register_callback_function (imap, MU_IMAP_CB_FETCH,
					      imap_fetch_callback,
					      mu_strout);
	  
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
    mu_error (_("Failed to create imap connection: %s"), mu_strerror (status));
  
  if (!status)
    {
      connect_argc = argc;
      connect_argv = mu_calloc (argc, sizeof (*connect_argv));
      for (i = 0; i < argc; i++)
	connect_argv[i] = mu_strdup (argv[i]);
      connect_argv[i] = NULL;
    
      imap_prompt_env ();
    }
  
  return status;
}

static int
com_starttls (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = mu_imap_starttls (imap);
  if (status)
    report_failure ("starttls", status);
  return 0;
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
    mu_printf (_("Try 'exit' to leave %s\n"), mu_program_name);
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
		mu_printf (_("%s is set\n"), argv[i]);
	      break;

	    case MU_ERR_NOENT:
	      mu_printf (_("%s is not set\n"), argv[i]);
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
      status = mu_getpass (mu_strin, mu_strout, _("Password:"), &passbuf);
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
    report_failure ("login", status);
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
	  mu_error (_("id -test requires an argument"));
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
	    mu_printf (_("%s is not set\n"), test);
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
      mu_imap_format_flags (mu_strout, st->defined_flags, 0);
      mu_printf ("\n");
    }
  if (st->flags & MU_IMAP_STAT_PERMANENT_FLAGS)
    {
      mu_printf (_("Flags permanent: "));
      mu_imap_format_flags (mu_strout, st->permanent_flags, 0);
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
    report_failure ("select", status);
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
    report_failure ("status", status);
  return 0;
}

static int
com_noop (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = mu_imap_noop (imap);
  if (status)
    report_failure ("noop", status);
  return 0;
}

static int
com_check (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = mu_imap_check (imap);
  if (status)
    report_failure ("check", status);
  return 0;
}

static int
com_fetch (int argc, char **argv)
{
  mu_stream_t out = mutool_open_pager ();
  mu_msgset_t msgset = parse_msgset (argv[1]);

  if (msgset)
    {
      int status;

      mu_imap_register_callback_function (imap, MU_IMAP_CB_FETCH,
					  imap_fetch_callback,
					  out);
      status = mu_imap_fetch (imap, uid_mode, msgset, argv[2]);
      mu_stream_destroy (&out);
      mu_imap_register_callback_function (imap, MU_IMAP_CB_FETCH,
					  imap_fetch_callback,
					  mu_strout);
      mu_msgset_free (msgset);
      if (status)
	report_failure ("fetch", status);
    }
  return 0;  
}

static int
com_store (int argc, char **argv)
{
  mu_msgset_t msgset = parse_msgset (argv[1]);

  if (msgset)
    {
      int status = mu_imap_store (imap, uid_mode, msgset, argv[2]);
      if (status)
	report_failure ("store", status);
    }
  return 0;
}

static int
com_close (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = mu_imap_close (imap);
  if (status)
    report_failure ("close", status);
  return 0;
}

static int
com_unselect (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = mu_imap_noop (imap);
  if (status)
    report_failure ("unselect", status);
  return 0;
}

static int
com_delete (int argc, char **argv)
{
  int status = mu_imap_delete (imap, argv[1]);
  if (status)
    report_failure ("delete", status);
  return 0;
}

static int
com_rename (int argc, char **argv)
{
  int status = mu_imap_rename (imap, argv[1], argv[2]);
  if (status)
    report_failure ("rename", status);
  return 0;
}

static int
com_expunge (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  int status = mu_imap_expunge (imap);
  if (status)
    report_failure ("expunge", status);
  return 0;
}

static int
com_create (int argc, char **argv)
{
  int status = mu_imap_mailbox_create (imap, argv[1]);
  if (status)
    report_failure ("create", status);
  return 0;
}

static int
com_subscribe (int argc, char **argv)
{
  int status = mu_imap_subscribe (imap, argv[1]);
  if (status)
    report_failure ("subscribe", status);
  return 0;
}

static int
com_unsubscribe (int argc, char **argv)
{
  int status = mu_imap_unsubscribe (imap, argv[1]);
  if (status)
    report_failure ("unsubscribe", status);
  return 0;
}

static int
com_append (int argc, char **argv)
{
  struct tm tmbuf, *tm = NULL;
  struct mu_timezone tzbuf, *tz = NULL;
  int flags = 0;
  mu_stream_t stream;
  int rc, i;
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-time") == 0)
	{
	  char *p;
	  
	  if (++i == argc)
	    {
	      mu_error (_("-time requires argument"));
	      return 0;
	    }
	  rc = mu_scan_datetime (argv[i], MU_DATETIME_INTERNALDATE,
				 &tmbuf, &tzbuf, &p);
	  if (rc || *p)
	    {
	      mu_error (_("cannot parse time"));
	      return 0;
	    }
	  tm = &tmbuf;
	  tz = &tzbuf;
	}
      else if (strcmp (argv[i], "-flag") == 0)
	{
	  if (++i == argc)
	    {
	      mu_error (_("-flag requires argument"));
	      return 0;
	    }
	  if (mu_imap_flag_to_attribute (argv[i], &flags))
	    {
	      mu_error (_("unrecognized flag: %s"), argv[i]);
	      return 0;
	    }
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  i++;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  mu_error (_("unrecognized option: %s"), argv[i]);
	  return 0;
	}
      else
	break;
    }

  if (argc - i != 2)
    {
      mu_error (_("wrong number of arguments"));
      return 0;
    }

  rc = mu_file_stream_create (&stream, argv[i + 1],
			      MU_STREAM_READ|MU_STREAM_SEEK);
  if (rc)
    {
      mu_error (_("cannot open file %s: %s"), argv[i + 1], mu_strerror (rc));
      return 0;
    }

  rc = mu_imap_append_stream (imap, argv[i], flags, tm, tz, stream);
  mu_stream_unref (stream);
  if (rc)
    report_failure ("append", rc);
  return 0;
}

static int
com_copy (int argc, char **argv)
{
  mu_msgset_t msgset = parse_msgset (argv[1]);
  if (msgset)
    {
      int status = mu_imap_copy (imap, uid_mode, msgset, argv[2]);
      mu_msgset_free (msgset);
      if (status)
	report_failure ("copy", status);
    }
  return 0;
}

static int
print_list_item (void *item, void *data)
{
  struct mu_list_response *resp = item;
  mu_stream_t out = data;

  mu_stream_printf (out,
		    "%c%c %c %4d %s\n",
		    (resp->type & MU_FOLDER_ATTRIBUTE_DIRECTORY) ? 'd' : '-',
		    (resp->type & MU_FOLDER_ATTRIBUTE_FILE) ? 'f' : '-',
		    resp->separator ?
		        resp->separator : MU_HIERARCHY_DELIMITER,
		    resp->level,
		    resp->name);
  return 0;
}

static int
com_list (int argc, char **argv)
{
  mu_list_t list;
  int rc;
  mu_stream_t out;
  
  rc = mu_imap_list_new (imap, argv[1], argv[2], &list);
  if (rc)
    {
      report_failure ("list", rc);
      return 0;
    }

  out = mutool_open_pager ();
  mu_list_foreach (list, print_list_item, out);
  mu_stream_unref (out);
  return 0;
}

static int
com_lsub (int argc, char **argv)
{
  mu_list_t list;
  int rc;
  mu_stream_t out;
  
  rc = mu_imap_lsub_new (imap, argv[1], argv[2], &list);
  if (rc)
    {
      report_failure ("lsub", rc);
      return 0;
    }

  out = mutool_open_pager ();
  mu_list_foreach (list, print_list_item, out);
  mu_stream_unref (out);
  return 0;
}

struct mutool_command imap_comtab[] = {
  { "capability",   1, -1, 0,
    com_capability,
    /* TRANSLATORS: -reread is a keyword; do not translate. */
    N_("[-reread] [NAME...]"),
    N_("list server capabilities") },
  { "verbose",      1, 4, 0,
    com_verbose,
    "[on|off|mask|unmask] [secure [payload]]",
    N_("control the protocol tracing") },
  { "connect",      1, 4, 0,
    com_connect,
    /* TRANSLATORS: -tls is a keyword. */
    N_("[-tls] HOSTNAME [PORT]"),
    N_("open connection") },
  { "disconnect",   1, 1, 0,
    com_disconnect,
    NULL,
    N_("close connection") },
  { "starttls",     1, 1, 0,
    com_starttls,
    NULL,
    N_("Establish TLS encrypted channel") },
  { "login",        2, 3, 0,
    com_login,
    N_("USER [PASS]"),
    N_("login to the server") },
  { "logout",       1, 1, 0,
    com_logout,
    NULL,
    N_("quit imap session") },
  { "id",           1, -1, 0,
    com_id,
    N_("[-test KW] [ARG [ARG...]]"),
    N_("send ID command") },
  { "noop",         1, 1, 0,
    com_noop,
    NULL,
    N_("no operation (keepalive)") },
  { "check",        1, 1, 0,
    com_check,
    NULL,
    N_("request a server checkpoint") },
  { "select",       1, 2, 0,
    com_select,
    N_("[MBOX]"),
    N_("select a mailbox") },
  { "examine",      1, 2, 0,
    com_examine,
    N_("[MBOX]"),
    N_("examine a mailbox") },
  { "status",       3, -1, 0,
    com_status,
    N_("MBOX KW [KW...]"),
    N_("get mailbox status") },
  { "fetch",        3, 3, CMD_COALESCE_EXTRA_ARGS,
    com_fetch,
    N_("MSGSET ITEMS"),
    N_("fetch message data") },
  { "store",        3, 3, CMD_COALESCE_EXTRA_ARGS,
    com_store,
    N_("MSGSET ITEMS"),
    N_("alter mailbox data") },
  { "close",        1, 1, 0,
    com_close,
    NULL,
    N_("close the mailbox (with expunge)") },
  { "unselect",     1, 1, 0,
    com_unselect,
    NULL,
    N_("close the mailbox (without expunge)") },
  { "delete",       2, 2, 0,
    com_delete,
    N_("MAILBOX"),
    N_("delete the mailbox") },
  { "rename",       3, 3, 0,
    com_rename,
    N_("OLD-NAME NEW-NAME"),
    N_("rename existing mailbox") },
  { "expunge",      1, 1, 0,
    com_expunge,
    NULL,
    N_("permanently remove messages marked for deletion") },
  { "create",       2, 2, 0,
    com_create,
    N_("MAILBOX"),
    N_("create new mailbox") },
  { "append",       3, -1, 0,
    com_append,
    N_("[-time DATETIME] [-flag FLAG] MAILBOX FILE"),
    N_("append message text from FILE to MAILBOX") },
  { "copy",         3, 3, 0,
    com_copy,
    N_("MSGSET MAILBOX"),
    N_("Copy messages from MSGSET to MAILBOX") },
  { "list",         3, 3, 0,
    com_list,
    N_("REF MBOX"),
    N_("List matching mailboxes") },
  { "lsub",         3, 3, 0,
    com_lsub,
    N_("REF MBOX"),
    N_("List subscribed mailboxes") },
  { "subscribe",    2, 2, 0,
    com_subscribe,
    N_("MBOX"),
    N_("Subscribe to a mailbox") },
  { "unsubscribe",    2, 2, 0,
    com_unsubscribe,
    N_("MBOX"),
    N_("Remove mailbox from subscription list") },
  { "uid",          1, 2, 0,
    com_uid,
    N_("[on|off]"),
    N_("control UID mode") },
  { "quit",         1, 1, 0,
    com_logout,
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
  mutool_shell_prompt = mu_strdup ("imap> ");
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

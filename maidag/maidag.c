/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

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

enum maidag_mode maidag_mode = mode_mda;
int multiple_delivery;     /* Don't return errors when delivering to multiple
			      recipients */
int ex_quota_tempfail;     /* Return temporary failure if mailbox quota is
			      exceeded. If this variable is not set, maidag
			      will return "service unavailable" */
int exit_code = EX_OK;     /* Exit code to be used */
uid_t current_uid;         /* Current user id */

char *quotadbname = NULL;  /* Name of mailbox quota database */
char *quota_query = NULL;  /* SQL query to retrieve mailbox quota */

char *sender_address = NULL;       

maidag_script_fun script_handler;

mu_list_t script_list;

char *forward_file = NULL;
#define FORWARD_FILE_PERM_CHECK (					\
			   MU_FILE_SAFETY_OWNER_MISMATCH |	\
			   MU_FILE_SAFETY_GROUP_WRITABLE |	\
			   MU_FILE_SAFETY_WORLD_WRITABLE |	\
			   MU_FILE_SAFETY_LINKED_WRDIR |		\
			   MU_FILE_SAFETY_DIR_IWGRP |		\
			   MU_FILE_SAFETY_DIR_IWOTH )
int forward_file_checks = FORWARD_FILE_PERM_CHECK;

/* Debuggig options */
int debug_level;           /* General debugging level */ 
int sieve_debug_flags;     /* Sieve debugging flags */
int sieve_enable_log;      /* Enables logging of executed Sieve actions */
char *message_id_header;   /* Use the value of this header as message
			      identifier when logging Sieve actions */

/* For LMTP mode */
mu_m_server_t server;
char *lmtp_url_string;
int reuse_lmtp_address = 1;
int maidag_transcript;

const char *program_version = "maidag (" PACKAGE_STRING ")";
static char doc[] =
N_("GNU maidag -- the mail delivery agent.")
"\v"
N_("Debug flags are:\n\
  g - guimb stack traces\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n\
  l - sieve action logs\n\
  0-9 - Set maidag debugging level\n");

static char args_doc[] = N_("[recipient...]");

#define STDERR_OPTION 256
#define MESSAGE_ID_HEADER_OPTION 257
#define LMTP_OPTION 258
#define FOREGROUND_OPTION 260
#define URL_OPTION 261
#define TRANSCRIPT_OPTION 262
#define MDA_OPTION 263

static struct argp_option options[] = 
{
#define GRID 0
 { NULL, 0, NULL, 0,
   N_("General options"), GRID },
      
 { "foreground", FOREGROUND_OPTION, 0, 0, N_("remain in foreground"),
   GRID + 1 },
 { "inetd",  'i', 0, 0, N_("run in inetd mode"), GRID + 1 },
 { "daemon", 'd', N_("NUMBER"), OPTION_ARG_OPTIONAL,
   N_("runs in daemon mode with a maximum of NUMBER children"), GRID + 1 },
 { "url", URL_OPTION, 0, 0, N_("deliver to given URLs"), GRID + 1 },
 { "mda", MDA_OPTION, 0, 0, N_("force MDA mode even if not started as root"),
   GRID + 1 },
 { "from", 'f', N_("EMAIL"), 0,
   N_("specify the sender's name"), GRID + 1 },
 { NULL, 'r', NULL, OPTION_ALIAS, NULL },
 { "lmtp", LMTP_OPTION, N_("URL"), OPTION_ARG_OPTIONAL,
   N_("operate in LMTP mode"), GRID + 1 },
 { "debug", 'x', N_("FLAGS"), 0,
   N_("enable debugging"), GRID + 1 },
 { "stderr", STDERR_OPTION, NULL, 0,
   N_("log to standard error"), GRID + 1 },
 { "transcript", TRANSCRIPT_OPTION, NULL, 0,
   N_("enable session transcript"), GRID + 1 },
#undef GRID

#define GRID 2
 { NULL, 0, NULL, 0,
   N_("Scripting options"), GRID },
 
  { "language", 'l', N_("STRING"), 0,
    N_("define scripting language for the next --script option"),
    GRID + 1 },
  { "script", 's', N_("PATTERN"), 0,
    N_("set name pattern for user-defined mail filter"), GRID + 1 },
  { "message-id-header", MESSAGE_ID_HEADER_OPTION, N_("STRING"), 0,
    N_("use this header to identify messages when logging Sieve actions"),
    GRID + 1 },
#undef GRID
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp argp = {
  options,
  parse_opt,
  args_doc, 
  doc,
  NULL,
  NULL, NULL
};

static const char *maidag_argp_capa[] = {
  "auth",
  "common",
  "debug",
  "logging",
  "mailbox",
  "locking",
  "mailer",
  NULL
};

#define D_DEFAULT "9,s"

static void
set_debug_flags (const char *arg)
{
  while (*arg)
    {
      if (mu_isdigit (*arg))
	debug_level = strtoul (arg, (char**)&arg, 10);
      else
	for (; *arg && *arg != ','; arg++)
	  {
	    switch (*arg)
	      {
	      case 'g':
#ifdef WITH_GUILE
		debug_guile = 1;
#endif
		break;

	      case 't':
		sieve_debug_flags |= MU_SIEVE_DEBUG_TRACE;
		break;
	  
	      case 'i':
		sieve_debug_flags |= MU_SIEVE_DEBUG_INSTR;
		break;
	  
	      case 'l':
		sieve_enable_log = 1;
		break;
	  
	      default:
		mu_error (_("%c is not a valid debug flag"), *arg);
		break;
	      }
	  }
      if (*arg == ',')
	arg++;
      else if (*arg)
	mu_error (_("expected comma, but found %c"), *arg);
    }
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static mu_list_t lst;

  switch (key)
    {
    case 'd':
      mu_argp_node_list_new (lst, "mode", "daemon");
      if (arg)
	mu_argp_node_list_new (lst, "max-children", arg);
      break;

    case 'i':
      mu_argp_node_list_new (lst, "mode", "inetd");
      break;

    case FOREGROUND_OPTION:
      mu_argp_node_list_new (lst, "foreground", "yes");
      break;
      
    case MESSAGE_ID_HEADER_OPTION:
      mu_argp_node_list_new (lst, "message-id-header", arg);
      break;

    case LMTP_OPTION:
      mu_argp_node_list_new (lst, "delivery-mode", "lmtp");
      if (arg)
	mu_argp_node_list_new (lst, "listen", arg);
      break;

    case MDA_OPTION:
      mu_argp_node_list_new (lst, "delivery-mode", "mda");
      break;
      
    case TRANSCRIPT_OPTION:
      maidag_transcript = 1;
      break;
      
    case 'r':
    case 'f':
      if (sender_address != NULL)
	argp_error (state, _("multiple --from options"));
      sender_address = arg;
      break;
      
    case 'l':
      script_handler = script_lang_handler (arg);
      if (!script_handler)
	argp_error (state, _("unknown or unsupported language: %s"),
		    arg);
      break;
      
    case 's':
      switch (script_register (arg))
	{
	case 0:
	  break;

	case EINVAL:
	  argp_error (state, _("%s has unknown file suffix"), arg);
	  break;

	default:
	  argp_error (state, _("error registering script"));
	}
      break;
      
    case 'x':
      mu_argp_node_list_new (lst, "debug", arg ? arg : D_DEFAULT);
      break;

    case STDERR_OPTION:
      mu_argp_node_list_new (lst, "stderr", "yes");
      break;

    case URL_OPTION:
      mu_argp_node_list_new (lst, "delivery-mode", "url");
      break;
      
    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;
      
    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (lst, NULL, NULL);
      break;
      
    case ARGP_KEY_ERROR:
      exit (EX_USAGE);

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}



static int
cb_debug (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  set_debug_flags (val->v.string);
  return 0;
}

static int
cb_stderr (void *data, mu_config_value_t *val)
{
  int res;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_cfg_parse_boolean (val->v.string, &res))
    mu_error (_("not a boolean"));
  else
    mu_log_syslog = !res;
  return 0;
}
    
static int
cb2_group (const char *gname, void *data)
{
  mu_list_t list = data;
  struct group *group;

  group = getgrnam (gname);
  if (!group)
    mu_error (_("unknown group: %s"), gname);
  else
    mu_list_append (list, (void*)group->gr_gid);
  return 0;
}
  
static int
cb_group (void *data, mu_config_value_t *arg)
{
  mu_list_t *plist = data;

  if (!*plist)
    mu_list_create (plist);
  return mu_cfg_string_value_cb (arg, cb2_group, *plist);
}

static int
cb2_forward_file_checks (const char *name, void *data)
{
  if (mu_file_safety_compose (data, name, FORWARD_FILE_PERM_CHECK))
    mu_error (_("unknown keyword: %s"), name);
  return 0;
}

static int
cb_forward_file_checks (void *data, mu_config_value_t *arg)
{
  return mu_cfg_string_value_cb (arg, cb2_forward_file_checks,
				 &forward_file_checks);
}

static int
cb_script_language (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  script_handler = script_lang_handler (val->v.string);
  if (!script_handler)
    {
      mu_error (_("unsupported language: %s"), val->v.string);
      return 1;
    }
  return 0;
}

static int
cb_script_pattern (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  
  switch (script_register (val->v.string))
    {
    case 0:
      break;

    case EINVAL:
      mu_error (_("%s has unknown file suffix"), val->v.string);
      break;

    default:
      mu_error (_("error registering script"));
    }
  return 0;
}

struct mu_cfg_param filter_cfg_param[] = {
  { "language", mu_cfg_callback, NULL, 0, cb_script_language,
    N_("Set script language.") },
  { "pattern", mu_cfg_callback, NULL, 0, cb_script_pattern,
    N_("Set script pattern.") },
  { NULL }
};

static int
cb_delivery_mode (void *data, mu_config_value_t *val)
{
  static mu_kwd_t mode_tab[] = {
    { "mda", mode_mda },
    { "url", mode_url },
    { "lmtp", mode_lmtp },
    { NULL }
  };
  int n;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;

  if (mu_kwd_xlat_name (mode_tab, val->v.string, &n) == 0)
    {
      maidag_mode = n;
      if (maidag_mode == mode_url)
	mu_log_syslog = 0;
    }
  else
    mu_error (_("%s is unknown"), val->v.string);
  return 0;
}

static int
cb_listen (void *data, mu_config_value_t *val)
{
  struct mu_sockaddr *s;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_m_server_parse_url (server, val->v.string, &s))
    return 1;
  mu_m_server_listen (server, s, MU_IP_TCP);
  return 0;
}

struct mu_cfg_param maidag_cfg_param[] = {
  { "delivery-mode", mu_cfg_callback, NULL, 0, cb_delivery_mode,
    N_("Set delivery mode"),
    N_("mode: {mda | url | lmtp}") },
  { "exit-multiple-delivery-success", mu_cfg_bool, &multiple_delivery, 0, NULL,
    N_("In case of multiple delivery, exit with code 0 if at least one "
       "delivery succeeded.") },
  { "exit-quota-tempfail", mu_cfg_bool, &ex_quota_tempfail, 0, NULL,
    N_("Indicate temporary failure if the recipient is over his mail quota.")
  },
#ifdef ENABLE_DBM
  { "quota-db", mu_cfg_string, &quotadbname, 0, NULL,
    N_("Name of DBM quota database file."),
    N_("file") },
#endif
#ifdef USE_SQL
  { "quota-query", mu_cfg_string, &quota_query, 0, NULL,
    N_("SQL query to retrieve mailbox quota.  This is deprecated, use "
       "sql { ... } instead."),
    N_("query") },
#endif
  { "message-id-header", mu_cfg_string, &message_id_header, 0, NULL,
    N_("When logging Sieve actions, identify messages by the value of "
       "this header."),
    N_("name") },
  { "debug", mu_cfg_callback, NULL, 0, cb_debug,
    N_("Set maidag debug level.  Debug level consists of one or more "
       "of the following letters:\n"
       "  g - guimb stack traces\n"
       "  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n"
       "  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n"
       "  l - sieve action logs\n") },
  { "stderr", mu_cfg_callback, NULL, 0, cb_stderr,
    N_("Log to stderr instead of syslog.") },
  { "forward-file", mu_cfg_string, &forward_file, 0, NULL,
    N_("Process forward file.") },
  { "forward-file-checks", mu_cfg_callback, NULL, 0, cb_forward_file_checks,
    N_("Configure safety checks for the forward file."),
    N_("arg: list") },
/* LMTP support */
  { "group", mu_cfg_callback, &lmtp_groups, 0, cb_group,
    N_("In LMTP mode, retain these supplementary groups."),
    N_("groups: list of string") },
  { "listen", mu_cfg_callback, NULL, 0, cb_listen,
    N_("In LMTP mode, listen on the given URL.  Valid URLs are:\n"
       "   tcp://<address: string>:<port: number> (note that port is "
       "mandatory)\n"
       "   file://<socket-file-name>\n"
       "or socket://<socket-file-name>"),
    N_("url") },
  { "reuse-address", mu_cfg_bool, &reuse_lmtp_address, 0, NULL,
    N_("Reuse existing address (LMTP mode).  Default is \"yes\".") },
  { "filter", mu_cfg_section, NULL, 0, NULL,
    N_("Add a message filter") },
  { ".server", mu_cfg_section, NULL, 0, NULL,
    N_("LMTP server configuration.") },
  TCP_WRAPPERS_CONFIG
  { NULL }
};

static void
maidag_cfg_init ()
{
  struct mu_cfg_section *section;
  if (mu_create_canned_section ("filter", &section) == 0)
    {
      section->docstring = N_("Add new message filter.");
      mu_cfg_section_add_params (section, filter_cfg_param);
    }
}

/* FIXME: These are for compatibility with MU 2.0.
   Remove in 2.2 */
extern mu_record_t mu_remote_smtp_record;
extern mu_record_t mu_remote_sendmail_record;
extern mu_record_t mu_remote_prog_record;


int
main (int argc, char *argv[])
{
  int arg_index;
  maidag_delivery_fn delivery_fun = NULL;
  
  /* Preparative work: close inherited fds, force a reasonable umask
     and prepare a logging. */
  close_fds ();
  umask (0077);

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  /* Default locker settings */
  mu_locker_set_default_flags (MU_LOCKER_PID|MU_LOCKER_RETRY,
			    mu_locker_assign);
  mu_locker_set_default_retry_timeout (1);
  mu_locker_set_default_retry_count (300);

  /* Register needed modules */
  MU_AUTH_REGISTER_ALL_MODULES ();

  /* Register all supported mailbox and mailer formats */
  mu_register_all_formats ();
  mu_registrar_record (mu_smtp_record);
  
  mu_gocs_register ("sieve", mu_sieve_module_init);

  mu_tcpwrapper_cfg_init ();
  mu_acl_cfg_init ();
  mu_m_server_cfg_init (NULL);
  maidag_cfg_init ();
  
  /* Parse command line */
#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif
  mu_argp_init (NULL, NULL);

  mu_m_server_create (&server, program_version);
  mu_m_server_set_conn (server, lmtp_connection);
  mu_m_server_set_prefork (server, mu_tcp_wrapper_prefork);
  mu_m_server_set_mode (server, MODE_INTERACTIVE);
  mu_m_server_set_max_children (server, 20);
  mu_m_server_set_timeout (server, 600);

  mu_log_syslog = -1;
  mu_log_print_severity = 1;
  
  if (mu_app_init (&argp, maidag_argp_capa, maidag_cfg_param, 
		   argc, argv, 0, &arg_index, server))
    exit (EX_CONFIG);

  current_uid = getuid ();

  if (mu_log_syslog == -1)
    {
      mu_log_syslog = !(maidag_mode == mode_url);
      mu_stdstream_strerr_setup (mu_log_syslog ?
				 MU_STRERR_SYSLOG : MU_STRERR_STDERR);
    }
  
  argc -= arg_index;
  argv += arg_index;

  switch (maidag_mode)
    {
    case mode_lmtp:
      if (argc)
	{
	  mu_error (_("too many arguments"));
	  return EX_USAGE;
	}
      return maidag_lmtp_server ();

    case mode_url:
      /* FIXME: Verify if the urls are deliverable? */
      delivery_fun = deliver_to_url;
      break;
      
    case mode_mda:
      if (argc == 0)
	{
	  mu_error (_("recipients not given"));
	  return EX_USAGE;
	}
      delivery_fun = deliver_to_user;
      break;
    }
  return maidag_stdio_delivery (delivery_fun, argc, argv);
}
  


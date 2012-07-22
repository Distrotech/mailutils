/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007-2012 Free Software Foundation, Inc.

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

#include "pop3d.h"
#include "mailutils/pam.h"
#include "mailutils/libargp.h"
#include "mailutils/pop3.h"
#include "mailutils/kwd.h"
#include "tcpwrap.h"

mu_mailbox_t mbox;
int state;
char *username;
char *md5shared;

mu_m_server_t server;
unsigned int idle_timeout;
int pop3d_transcript;
int debug_mode;
int pop3d_xlines;
char *apop_database_name = APOP_PASSFILE;
int apop_database_safety = MU_FILE_SAFETY_ALL;
uid_t apop_database_owner;
int apop_database_owner_set;

enum tls_mode tls_mode;

#ifdef WITH_TLS
int tls_available;
int tls_done;
#endif /* WITH_TLS */

int initial_state = AUTHORIZATION; 

/* Should all the messages be undeleted on startup */
int undelete_on_startup;
#ifdef ENABLE_LOGIN_DELAY
/* Minimum allowed delay between two successive logins */
time_t login_delay = 0;
char *login_stat_file = LOGIN_STAT_FILE;
#endif

unsigned expire = EXPIRE_NEVER; /* Expire messages after this number of days */
int expire_on_exit = 0;         /* Delete expired messages on exit */

static error_t pop3d_parse_opt  (int key, char *arg, struct argp_state *astate);

const char *program_version = "pop3d (" PACKAGE_STRING ")";
static char doc[] = N_("GNU pop3d -- the POP3 daemon.");

#define OPT_FOREGROUND      256

static struct argp_option options[] = {
#define GRP 0
  { "foreground", OPT_FOREGROUND, 0, 0, N_("remain in foreground"), GRP+1},
  { "inetd",  'i', 0, 0, N_("run in inetd mode"), GRP+1},
  { "daemon", 'd', N_("NUMBER"), OPTION_ARG_OPTIONAL,
    N_("runs in daemon mode with a maximum of NUMBER children"), GRP+1 },
#undef GRP

  {NULL, 0, NULL, 0, NULL, 0}
};

static int
cb_bulletin_source (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  set_bulletin_source (val->v.string); /* FIXME: Error reporting? */
  return 0;
}

static int
cb2_file_safety_checks (const char *name, void *data)
{
  if (mu_file_safety_compose (data, name, MU_FILE_SAFETY_ALL))
    mu_error (_("unknown keyword: %s"), name);
  return 0;
}

static int
cb_apop_safety_checks (void *data, mu_config_value_t *arg)
{
  return mu_cfg_string_value_cb (arg, cb2_file_safety_checks,
				 &apop_database_safety);
}

static int
cb_apop_database_owner (void *data, mu_config_value_t *val)
{
  struct passwd *pw;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  pw = getpwnam (val->v.string);
  if (!pw)
    {
      char *p;
      unsigned long n;
    
      n = strtoul (val->v.string, &p, 10);
      if (*p)
	{
	  mu_error (_("no such user: %s"), val->v.string);
	  return 1;
	}
      apop_database_owner = n;
    }
  else
    apop_database_owner = pw->pw_uid;
  apop_database_owner_set = 1;
  return 0;
}

#ifdef ENABLE_DBM
static int
cb_bulletin_db (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  set_bulletin_db (val->v.string); /* FIXME: Error reporting? */
  return 0;
}
#endif

struct pop3d_srv_config
{
  struct mu_srv_config m_cfg;
  enum tls_mode tls_mode;
};

#ifdef WITH_TLS
static int
cb_tls (void *data, mu_config_value_t *val)
{
  int *res = data;
  static struct mu_kwd tls_kwd[] = {
    { "no", tls_no },
    { "false", tls_no },
    { "off", tls_no },
    { "0", tls_no },
    { "ondemand", tls_ondemand },
    { "stls", tls_ondemand },
    { "required", tls_required },
    { "connection", tls_connection },
    /* For compatibility with prior versions: */
    { "yes", tls_connection }, 
    { "true", tls_connection },
    { "on", tls_connection },
    { "1", tls_connection },
    { NULL }
  };
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;

  if (mu_kwd_xlat_name (tls_kwd, val->v.string, res))
    mu_error (_("not a valid tls keyword: %s"), val->v.string);
  return 0;
}
#endif

static int
cb_tls_required (void *data, mu_config_value_t *val)
{
  int *res = data;
  int bv;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_cfg_parse_boolean (val->v.string, &bv))
    mu_error (_("Not a boolean value"));
  else
    *res = tls_required;
  return 0;
}

static struct mu_cfg_param pop3d_srv_param[] = {
#ifdef WITH_TLS
  { "tls", mu_cfg_callback,
    NULL, mu_offsetof (struct pop3d_srv_config, tls_mode), cb_tls,
    N_("Kind of TLS encryption to use for this server") },
#endif
  { NULL }
};
    
static struct mu_cfg_param pop3d_cfg_param[] = {
  { "undelete", mu_cfg_bool, &undelete_on_startup, 0, NULL,
    N_("On startup, clear deletion marks from all the messages.") },
  { "expire", mu_cfg_uint, &expire, 0, NULL,
    N_("Automatically expire read messages after the given number of days."),
    N_("days") },
  { "delete-expired", mu_cfg_bool, &expire_on_exit, 0, NULL,
    N_("Delete expired messages upon closing the mailbox.") },
  { "scan-lines", mu_cfg_bool, &pop3d_xlines, 0, NULL,
    N_("Output the number of lines in the message in its scan listing.") },
  { "apop-database-file", mu_cfg_string, &apop_database_name, 0, NULL,
    N_("set APOP database file name or URL") },
  { "apop-database-owner", mu_cfg_callback, NULL, 0, cb_apop_database_owner,
    N_("Name or UID of the APOP database owner") },
  { "apop-database-safety", mu_cfg_callback, NULL, 0, cb_apop_safety_checks,
    N_("Configure safety checks for APOP database files.  Argument is a list or "
       "sequence of check names optionally prefixed with '+' to enable or "
       "'-' to disable the corresponding check.  Valid check names are:\n"
       "\n"
       "  none          disable all checks\n"
       "  all           enable all checks\n"
       "  gwrfil        forbid group writable files\n"
       "  awrfil        forbid world writable files\n"
       "  grdfil        forbid group readable files\n"
       "  ardfil        forbid world writable files\n"
       "  linkwrdir     forbid symbolic links in group or world writable directories\n"
       "  gwrdir        forbid files in group writable directories\n"
       "  awrdir        forbid files in world writable directories\n"),
    N_("arg: list") },  
    
#ifdef WITH_TLS
  { "tls", mu_cfg_callback, &tls_mode, 0, cb_tls,
    N_("Kind of TLS encryption to use") },
  { "tls-required", mu_cfg_callback, &tls_mode, 0, cb_tls_required,
    N_("Always require STLS before entering authentication phase.\n"
       "Deprecated, use \"tls required\" instead.") },
#endif
#ifdef ENABLE_LOGIN_DELAY
  { "login-delay", mu_cfg_time, &login_delay, 0, NULL,
    N_("Set the minimal allowed delay between two successive logins.") },
  { "stat-file", mu_cfg_string, &login_stat_file, 0, NULL,
    N_("Set the name of login statistics file (for login-delay).") },
#endif
  { "bulletin-source", mu_cfg_callback, NULL, 0, cb_bulletin_source,
    N_("Get bulletins from the specified mailbox."),
    N_("url") },
#ifdef ENABLE_DBM
  { "bulletin-db", mu_cfg_callback, NULL, 0, cb_bulletin_db,
    N_("Set the bulletin database file name."),
    N_("file") },
#endif
  { "output-buffer-size", mu_cfg_size, &pop3d_output_bufsize, 0, NULL,
    N_("Size of the output buffer.") },
  { "mandatory-locking", mu_cfg_section },
  { ".server", mu_cfg_section, NULL, 0, NULL,
    N_("Server configuration.") },
  { "transcript", mu_cfg_bool, &pop3d_transcript, 0, NULL,
    N_("Set global transcript mode.") },
  TCP_WRAPPERS_CONFIG
  { NULL }
};
    
static struct argp argp = {
  options,
  pop3d_parse_opt,
  NULL,
  doc,
  NULL,
  NULL, NULL
};

static const char *pop3d_argp_capa[] = {
  "auth",
  "common",
  "debug",
  "mailbox",
  "locking",
  "logging",
  NULL
};

static error_t
pop3d_parse_opt (int key, char *arg, struct argp_state *astate)
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

    case OPT_FOREGROUND:
      mu_argp_node_list_new (lst, "foreground", "yes");
      break;
      
    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;
      
    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (lst, NULL, NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
pop3d_get_client_address (int fd, struct sockaddr_in *pcs)
{
  mu_diag_output (MU_DIAG_INFO, _("incoming connection opened"));

  /* log information on the connecting client. */
  if (debug_mode)
    {
      mu_diag_output (MU_DIAG_INFO, _("started in debugging mode"));
      return 1;
    }
  else
    {
      socklen_t len = sizeof *pcs;
      if (getpeername (fd, (struct sockaddr*) pcs, &len) < 0)
	{
	  mu_diag_output (MU_DIAG_ERROR,
			  _("cannot obtain IP address of client: %s"),
			  strerror (errno));
	  return 1;
	}
    }
  return 0;
}

/* The main part of the daemon. This function reads input from the client and
   executes the proper functions. Also handles the bulk of error reporting.
   Arguments:
      ifd       --  input descriptor
      ofd       --  output descriptor
      tls       --  initiate encrypted connection */
int
pop3d_mainloop (int ifd, int ofd, enum tls_mode tls)
{
  int status = OK;
  char buffer[512];
  static int sigtab[] = { SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTOP, SIGPIPE,
			  SIGABRT, SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGALRM };
  struct pop3d_session session;
     
  mu_set_signals (pop3d_child_signal, sigtab, MU_ARRAY_SIZE (sigtab));

  if (tls == tls_unspecified)
    tls = tls_available ? tls_ondemand : tls_no;
  else if (tls != tls_no && !tls_available)
    {
      mu_error (_("TLS is not configured, but requested in the "
		  "configuration"));
      tls = tls_no;
    }
  
  pop3d_setio (ifd, ofd, tls == tls_connection);

  if (tls == tls_required)
    initial_state = INITIAL;
  
  state = tls == tls_connection ? AUTHORIZATION : initial_state;

  pop3d_session_init (&session);
  session.tls = tls;
  /* FIXME: state should also be in the session? */
  
  /* Prepare the shared secret for APOP.  */
  {
    char *local_hostname;
    
    status = mu_get_host_name (&local_hostname);
    if (status)
      {
        mu_diag_funcall (MU_DIAG_ERROR, "mu_get_host_name", NULL, status);
        exit (EXIT_FAILURE);
      }

    md5shared = mu_alloc (strlen (local_hostname) + 51);

    snprintf (md5shared, strlen (local_hostname) + 50, "<%u.%u@%s>", getpid (),
	      (unsigned)time (NULL), local_hostname);
    free (local_hostname);
  }

  /* Lets boogie.  */
  pop3d_outf ("+OK POP3 Ready %s\n", md5shared);

  while (state != UPDATE && state != ABORT)
    {
      char *buf;
      char *arg, *cmd;
      pop3d_command_handler_t handler;
      
      pop3d_flush_output ();
      status = OK;
      buf = pop3d_readline (buffer, sizeof (buffer));
      pop3d_parse_command (buf, &cmd, &arg);

      if (state == TRANSACTION && !mu_mailbox_is_updated (mbox))
	{
	  static mu_off_t mailbox_size;
	  mu_off_t newsize = 0;
	  mu_mailbox_get_size (mbox, &newsize);
	  /* Did we shrink?  First time save the size.  */
	  if (!mailbox_size)
	    mailbox_size = newsize;
	  else if (newsize < mailbox_size) /* FIXME: Should it be a != ? */
	    pop3d_abquit (ERR_MBOX_SYNC); /* Out of sync, Bail out.  */
	}

      /* Refresh the Lock.  */
      manlock_touchlock (mbox);

      if ((handler = pop3d_find_command (cmd)) != NULL)
	status = handler (arg, &session);
      else
	status = ERR_BAD_CMD;

      if (status != OK)
	pop3d_outf ("-ERR %s\n", pop3d_error_string (status));
    }

  pop3d_session_free (&session);
  
  pop3d_bye ();

  return status;
}

static int
set_strerr_flt ()
{
  mu_stream_t flt, trans[2];
  int rc;
  
  rc = mu_stream_ioctl (mu_strerr, MU_IOCTL_TOPSTREAM, MU_IOCTL_OP_GET, trans);
  if (rc == 0)
    {
      char sessidstr[10];
      char *argv[] = { "inline-comment", NULL, "-S", NULL };

      snprintf (sessidstr, sizeof sessidstr, "%08lx:", mu_session_id);
      argv[1] = sessidstr;
      rc = mu_filter_create_args (&flt, trans[0], "inline-comment", 3,
				  (const char **)argv,
				  MU_FILTER_ENCODE, MU_STREAM_WRITE);
      mu_stream_unref (trans[0]);
      if (rc == 0)
	{
	  mu_stream_set_buffer (flt, mu_buffer_line, 0);
	  trans[0] = flt;
	  trans[1] = NULL;
	  rc = mu_stream_ioctl (mu_strerr, MU_IOCTL_TOPSTREAM,
				MU_IOCTL_OP_SET, trans);
	  mu_stream_unref (trans[0]);
	  if (rc)
	    mu_error (_("%s failed: %s"), "MU_IOCTL_SET_STREAM",
		      mu_stream_strerror (mu_strerr, rc));
	}
      else
	mu_error (_("cannot create log filter stream: %s"), mu_strerror (rc));
    }
  else
    {
      mu_error (_("%s failed: %s"), "MU_IOCTL_GET_STREAM",
		mu_stream_strerror (mu_strerr, rc));
    }
  return rc;
}

static void
clr_strerr_flt ()
{
  mu_stream_t flt, trans[2];
  int rc;

  rc = mu_stream_ioctl (mu_strerr, MU_IOCTL_TOPSTREAM, MU_IOCTL_OP_GET, trans);
  if (rc == 0)
    {
      flt = trans[0];

      rc = mu_stream_ioctl (flt, MU_IOCTL_TOPSTREAM, MU_IOCTL_OP_GET, trans);
      if (rc == 0)
	{
	  mu_stream_unref (trans[0]);
	  rc = mu_stream_ioctl (mu_strerr, MU_IOCTL_TOPSTREAM,
				MU_IOCTL_OP_SET, trans);
	  if (rc == 0)
	    mu_stream_unref (flt);
	}
    }
}

int
pop3d_connection (int fd, struct sockaddr *sa, int salen,
		  struct mu_srv_config *pconf,
		  void *data)
{
  struct pop3d_srv_config *cfg = (struct pop3d_srv_config *) pconf;
  int rc;
  
  idle_timeout = pconf->timeout;
  pop3d_transcript = pconf->transcript;

  if (mu_log_session_id)
    rc = set_strerr_flt ();
  else
    rc = 1;

  pop3d_mainloop (fd, fd,
		  cfg->tls_mode == tls_unspecified ? tls_mode : cfg->tls_mode);

  if (rc == 0)
    clr_strerr_flt ();
  return 0;
}

static void
pop3d_alloc_die ()
{
  pop3d_abquit (ERR_NO_MEM);
}

#ifdef ENABLE_DBM
static void
set_dbm_safety ()
{
  mu_url_t hints = mu_dbm_get_hint ();
  const char *param[] = { "+all" };
  mu_url_add_param (hints, 1, param);
}
#endif

int
main (int argc, char **argv)
{
  struct group *gr;
  int status = OK;
  static int sigtab[] = { SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTOP, SIGPIPE };

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  MU_AUTH_REGISTER_ALL_MODULES();
  /* Register the desired formats.  */
  mu_register_local_mbox_formats ();

#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif /* WITH_TLS */
  mu_tcpwrapper_cfg_init ();
  manlock_cfg_init ();
  mu_acl_cfg_init ();
  mu_m_server_cfg_init (pop3d_srv_param);
  
  mu_argp_init (NULL, NULL);
  	
  mu_m_server_create (&server, program_version);
  mu_m_server_set_config_size (server, sizeof (struct pop3d_srv_config));
  mu_m_server_set_conn (server, pop3d_connection);
  mu_m_server_set_prefork (server, mu_tcp_wrapper_prefork);
  mu_m_server_set_mode (server, MODE_INTERACTIVE);
  mu_m_server_set_max_children (server, 20);
  /* FIXME mu_m_server_set_pidfile (); */
  mu_m_server_set_default_port (server, 110);
  mu_m_server_set_timeout (server, 600);
  mu_m_server_set_strexit (server, mu_strexit);

  mu_alloc_die_hook = pop3d_alloc_die;

  mu_log_syslog = 1;
  manlock_mandatory_locking = 1;

#ifdef ENABLE_DBM
  set_dbm_safety ();
#endif
  
  if (mu_app_init (&argp, pop3d_argp_capa, pop3d_cfg_param, 
		   argc, argv, 0, NULL, server))
    exit (EX_CONFIG); /* FIXME: No way to discern from EX_USAGE? */

  if (expire == 0)
    expire_on_exit = 1;

#ifdef USE_LIBPAM
  if (!mu_pam_service)
    mu_pam_service = "gnu-pop3d";
#endif

  if (mu_m_server_mode (server) == MODE_INTERACTIVE && isatty (0))
    {
      /* If input is a tty, switch to debug mode */
      debug_mode = 1;
    }
  else
    {
      errno = 0;
      gr = getgrnam ("mail");
      if (gr == NULL)
	{
	  if (errno == 0 || errno == ENOENT)
            {
               mu_error (_("%s: no such group"), "mail");
               exit (EX_CONFIG);
            }
          else
            {
	      mu_diag_funcall (MU_DIAG_ERROR, "getgrnam", "mail", errno);
	      exit (EX_OSERR);
            }
	}

      if (setgid (gr->gr_gid) == -1)
	{
	  mu_error (_("error setting mail group: %s"), mu_strerror (errno));
	  exit (EX_OSERR);
	}
    }

  /* Set the signal handlers.  */
  mu_set_signals (pop3d_master_signal, sigtab, MU_ARRAY_SIZE (sigtab));

  mu_stdstream_strerr_setup (mu_log_syslog ?
			     MU_STRERR_SYSLOG : MU_STRERR_STDERR);
  
  umask (S_IROTH | S_IWOTH | S_IXOTH);	/* 007 */

  /* Check TLS environment, i.e. cert and key files */
#ifdef WITH_TLS
  tls_available = mu_check_tls_environment ();
  if (tls_available)
    {
      tls_available = mu_init_tls_libs (1);
      if (tls_available)
	enable_stls ();
    }
#endif /* WITH_TLS */

  /* Actually run the daemon.  */
  if (mu_m_server_mode (server) == MODE_DAEMON)
    {
      mu_m_server_begin (server);
      status = mu_m_server_run (server);
      mu_m_server_end (server);
      mu_m_server_destroy (&server);
    }
  else
    {
      /* Make sure we are in the root directory.  */
      chdir ("/");
      status = pop3d_mainloop (MU_STDIN_FD, MU_STDOUT_FD, tls_no);
    }
  
  if (status)
    mu_error (_("main loop status: %s"), mu_strerror (status));	  
  /* Close the syslog connection and exit.  */
  closelog ();
  return status ? EX_SOFTWARE : EX_OK;
}


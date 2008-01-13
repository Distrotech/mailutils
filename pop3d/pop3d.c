/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 
   2005, 2007, 2008 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "pop3d.h"
#include "mailutils/pam.h"
#include "mailutils/libargp.h"
#include "tcpwrap.h"

mu_mailbox_t mbox;
int state;
char *username;
char *md5shared;

mu_m_server_t server;
unsigned int idle_timeout;
int pop3d_transcript;
int debug_mode;

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
int expire_on_exit = 0;       /* Delete expired messages on exit */

static error_t pop3d_parse_opt  (int key, char *arg, struct argp_state *astate);

const char *program_version = "pop3d (" PACKAGE_STRING ")";
static char doc[] = N_("GNU pop3d -- the POP3 daemon");

#define OPT_LOGIN_DELAY     257
#define OPT_STAT_FILE       258
#define OPT_EXPIRE          259
#define OPT_EXPIRE_ON_EXIT  260
#define OPT_TLS_REQUIRED    261
#define OPT_BULLETIN_SOURCE 262
#define OPT_BULLETIN_DB     263
#define OPT_FOREGROUND      264

static struct argp_option options[] = {
#define GRP 0
  { "foreground", OPT_FOREGROUND, 0, 0, N_("Remain in foreground."), GRP+1},
  { "inetd",  'i', 0, 0, N_("Run in inetd mode"), GRP+1},
  { "daemon", 'd', N_("NUMBER"), OPTION_ARG_OPTIONAL,
    N_("Runs in daemon mode with a maximum of NUMBER children"), GRP+1 },
#undef GRP

#define GRP 5
  {"undelete", 'u', NULL, OPTION_HIDDEN,
   N_("Undelete all messages on startup"), GRP+1},
  {"expire", OPT_EXPIRE, N_("DAYS"), OPTION_HIDDEN,
   N_("Expire read messages after the given number of days"), GRP+1},
  {"delete-expired", OPT_EXPIRE_ON_EXIT, NULL, OPTION_HIDDEN,
   N_("Delete expired messages upon closing the mailbox"), GRP+1},
#ifdef WITH_TLS
  {"tls-required", OPT_TLS_REQUIRED, NULL, OPTION_HIDDEN,
   N_("Always require STLS before entering authentication phase")},
#endif
#undef GRP
  
#define GRP 10
#ifdef ENABLE_LOGIN_DELAY
  {"login-delay", OPT_LOGIN_DELAY, N_("SECONDS"), OPTION_HIDDEN,
   N_("Allowed delay between two successive logins"), GRP+1},
  {"stat-file", OPT_STAT_FILE, N_("FILENAME"), OPTION_HIDDEN,
   N_("Name of login statistics file"), GRP+1},
#endif
  
#undef GRP

#define GRP 20
  { "bulletin-source", OPT_BULLETIN_SOURCE, N_("MBOX"), OPTION_HIDDEN,
    N_("Set source mailbox to get bulletins from"), GRP+1 },
#ifdef USE_DBM
  { "bulletin-db", OPT_BULLETIN_DB, N_("FILE"), OPTION_HIDDEN,
    N_("Set the bulletin database file name"), GRP+1 },
#endif
#undef GRP
  
  {NULL, 0, NULL, 0, NULL, 0}
};

#ifdef WITH_TLS
static int
cb_tls_required (mu_debug_t debug, void *data, char *arg)
{
  initial_state = INITIAL;
  return 0;
}
#endif

static int
cb_bulletin_source (mu_debug_t debug, void *data, char *arg)
{
  set_bulletin_source (arg); /* FIXME: Error reporting? */
  return 0;
}

static int
cb_bulletin_db (mu_debug_t debug, void *data, char *arg)
{
  set_bulletin_db (arg); /* FIXME: Error reporting? */
  return 0;
}

static struct mu_cfg_param pop3d_cfg_param[] = {
  { "undelete", mu_cfg_int, &undelete_on_startup, 0, NULL,
    N_("On startup, clear deletion marks from all the messages.") },
  { "expire", mu_cfg_uint, &expire, 0, NULL,
    N_("Automatically expire read messages after the given number of days."),
    N_("days") },
  { "delete-expired", mu_cfg_int, &expire_on_exit, 0, NULL,
    N_("Delete expired messages upon closing the mailbox.") },
#ifdef WITH_TLS
  { "tls-required", mu_cfg_callback, NULL, 0, cb_tls_required,
     N_("Always require STLS before entering authentication phase.") },
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
#ifdef USE_DBM
  { "bulletin-db", mu_cfg_callback, NULL, 0, cb_bulletin_db,
    N_("Set the bulletin database file name."),
    N_("file") },
#endif
  { ".server", mu_cfg_section, NULL, 0, NULL,
    N_("Server configuration.") },
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
  "license",
  NULL
};

static error_t
pop3d_parse_opt (int key, char *arg, struct argp_state *astate)
{
  static struct mu_argp_node_list lst;
  
  switch (key)
    {
    case 'd':
      mu_argp_node_list_new (&lst, "mode", "daemon");
      if (arg)
	mu_argp_node_list_new (&lst, "max-children", arg);
      break;

    case 'i':
      mu_argp_node_list_new (&lst, "mode", "inetd");
      break;

    case OPT_FOREGROUND:
      mu_argp_node_list_new (&lst, "foreground", "yes");
      break;
      
    case 'u':
      mu_argp_node_list_new (&lst, "undelete", "yes");
      break;

#ifdef ENABLE_LOGIN_DELAY
    case OPT_LOGIN_DELAY:
      mu_argp_node_list_new (&lst, "login-delay", arg);
      break;

    case OPT_STAT_FILE:
      mu_argp_node_list_new (&lst, "stat-file", arg);
      break;
#endif  
 
    case OPT_EXPIRE:
      mu_argp_node_list_new (&lst, "expire", arg);
      break;

    case OPT_EXPIRE_ON_EXIT:
      mu_argp_node_list_new (&lst, "delete-expired", "yes");
      break;

#ifdef WITH_TLS
    case OPT_TLS_REQUIRED:
      mu_argp_node_list_new (&lst, "tls-required", "yes");
      break;
#endif

    case OPT_BULLETIN_SOURCE:
      mu_argp_node_list_new (&lst, "bulletin-source", arg);
      break;
      
#ifdef USE_DBM
    case OPT_BULLETIN_DB:
      mu_argp_node_list_new (&lst, "bulletin-db", arg);
      break;
#endif
      
    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;
      
    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (&lst, NULL, NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
pop3d_get_client_address (int fd, struct sockaddr_in *pcs)
{
  mu_diag_output (MU_DIAG_INFO, _("Incoming connection opened"));

  /* log information on the connecting client. */
  if (debug_mode)
    {
      mu_diag_output (MU_DIAG_INFO, _("Started in debugging mode"));
      return 1;
    }
  else
    {
      int len = sizeof *pcs;
      if (getpeername (fd, (struct sockaddr*) pcs, &len) < 0)
	{
	  mu_diag_output (MU_DIAG_ERROR,
			  _("Cannot obtain IP address of client: %s"),
			  strerror (errno));
	  return 1;
	}
    }
  return 0;
}

/* The main part of the daemon. This function reads input from the client and
   executes the proper functions. Also handles the bulk of error reporting.
   Arguments:
      fd        --  socket descriptor (for diagnostics)
      infile    --  input stream
      outfile   --  output stream */
int
pop3d_mainloop (int fd, FILE *infile, FILE *outfile)
{
  int status = OK;
  char buffer[512];
    
  /* Reset hup to exit.  */
  signal (SIGHUP, pop3d_signal);
  /* Timeout alarm.  */
  signal (SIGALRM, pop3d_signal);

  pop3d_setio (infile, outfile);

  state = initial_state;

  /* Prepare the shared secret for APOP.  */
  {
    char *local_hostname;
    local_hostname = malloc (MAXHOSTNAMELEN + 1);
    if (local_hostname == NULL)
      pop3d_abquit (ERR_NO_MEM);

    /* Get our canonical hostname. */
    {
      struct hostent *htbuf;
      gethostname (local_hostname, MAXHOSTNAMELEN);
      htbuf = gethostbyname (local_hostname);
      if (htbuf)
	{
	  free (local_hostname);
	  local_hostname = strdup (htbuf->h_name);
	}
    }

    md5shared = malloc (strlen (local_hostname) + 51);
    if (md5shared == NULL)
      pop3d_abquit (ERR_NO_MEM);

    snprintf (md5shared, strlen (local_hostname) + 50, "<%u.%u@%s>", getpid (),
	      (unsigned)time (NULL), local_hostname);
    free (local_hostname);
  }

  /* Lets boogie.  */
  pop3d_outf ("+OK POP3 Ready %s\r\n", md5shared);

  while (state != UPDATE)
    {
      char *buf, *arg, *cmd;

      pop3d_flush_output ();
      status = OK;
      buf = pop3d_readline (buffer, sizeof (buffer));
      cmd = pop3d_cmd (buf);
      arg = pop3d_args (buf);

      /* The mailbox size needs to be check to make sure that we are in
	 sync.  Some other applications may not respect the *.lock or
	 the lock may be stale because downloading on slow modem.
	 We rely on the size of the mailbox for the check and bail if out
	 of sync.  */
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
      pop3d_touchlock ();

      if (strlen (arg) > POP_MAXCMDLEN || strlen (cmd) > POP_MAXCMDLEN)
	status = ERR_TOO_LONG;
      else if (strlen (cmd) > 4)
	status = ERR_BAD_CMD;
      else if (strncasecmp (cmd, "RETR", 4) == 0)
	status = pop3d_retr (arg);
      else if (strncasecmp (cmd, "DELE", 4) == 0)
	status = pop3d_dele (arg);
      else if (strncasecmp (cmd, "USER", 4) == 0)
	status = pop3d_user (arg);
      else if (strncasecmp (cmd, "QUIT", 4) == 0)
	status = pop3d_quit (arg);
      else if (strncasecmp (cmd, "APOP", 4) == 0)
	status = pop3d_apop (arg);
      else if (strncasecmp (cmd, "AUTH", 4) == 0)
	status = pop3d_auth (arg);
      else if (strncasecmp (cmd, "STAT", 4) == 0)
	status = pop3d_stat (arg);
      else if (strncasecmp (cmd, "LIST", 4) == 0)
	status = pop3d_list (arg);
      else if (strncasecmp (cmd, "NOOP", 4) == 0)
	status = pop3d_noop (arg);
      else if (strncasecmp (cmd, "RSET", 4) == 0)
	status = pop3d_rset (arg);
      else if ((strncasecmp (cmd, "TOP", 3) == 0) && (strlen (cmd) == 3))
	status = pop3d_top (arg);
      else if (strncasecmp (cmd, "UIDL", 4) == 0)
	status = pop3d_uidl (arg);
      else if (strncasecmp (cmd, "CAPA", 4) == 0)
	status = pop3d_capa (arg);
#ifdef WITH_TLS
      else if ((strncasecmp (cmd, "STLS", 4) == 0) && tls_available)
	{
	  status = pop3d_stls (arg);
	  if (status)
	    {
	      mu_diag_output (MU_DIAG_ERROR, _("Session terminated"));
	      break;
	    }
	}
#endif /* WITH_TLS */
      else
	status = ERR_BAD_CMD;

      if (status == OK)
	; /* Everything is good.  */
      else if (status == ERR_WRONG_STATE)
	pop3d_outf ("-ERR " BAD_STATE "\r\n");
      else if (status == ERR_BAD_ARGS)
	pop3d_outf ("-ERR " BAD_ARGS "\r\n");
      else if (status == ERR_NO_MESG)
	pop3d_outf ("-ERR " NO_MESG "\r\n");
      else if (status == ERR_MESG_DELE)
	pop3d_outf ("-ERR " MESG_DELE "\r\n");
      else if (status == ERR_NOT_IMPL)
	pop3d_outf ("-ERR " NOT_IMPL "\r\n");
      else if (status == ERR_BAD_CMD)
	pop3d_outf ("-ERR " BAD_COMMAND "\r\n");
      else if (status == ERR_BAD_LOGIN)
	pop3d_outf ("-ERR " BAD_LOGIN "\r\n");
      else if (status == ERR_MBOX_LOCK)
	pop3d_outf ("-ERR [IN-USE] " MBOX_LOCK "\r\n");
      else if (status == ERR_TOO_LONG)
	pop3d_outf ("-ERR " TOO_LONG "\r\n");
      else if (status == ERR_FILE)
	pop3d_outf ("-ERR " FILE_EXP "\r\n");
#ifdef WITH_TLS
      else if (status == ERR_TLS_ACTIVE)
	pop3d_outf ("-ERR " TLS_ACTIVE "\r\n");
#endif /* WITH_TLS */
      else if (status == ERR_LOGIN_DELAY)
	pop3d_outf ("-ERR [LOGIN-DELAY] " LOGIN_DELAY "\r\n");
      else
	pop3d_outf ("-ERR unknown error\r\n");

      free (cmd);
      free (arg);
    }

  pop3d_bye ();

  return (status != OK);
}

int
pop3d_connection (int fd, struct sockaddr *sa, int salen, void *data,
		  mu_ip_server_t srv, time_t timeout, int transcript)
{
  idle_timeout = timeout;
  pop3d_transcript = transcript;
  pop3d_mainloop (fd, fdopen (fd, "r"), fdopen (fd, "w"));
  return 0;
}

int
main (int argc, char **argv)
{
  struct group *gr;
  int status = OK;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  MU_AUTH_REGISTER_ALL_MODULES();
  /* Register the desired formats.  */
  mu_register_local_mbox_formats ();

#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif /* WITH_TLS */
  mu_tcpwrapper_cfg_init ();
  mu_acl_cfg_init ();
  mu_m_server_cfg_init ();
  
  mu_argp_init (program_version, NULL);
  	
  mu_m_server_create (&server, "GNU pop3d");
  mu_m_server_set_conn (server, pop3d_connection);
  mu_m_server_set_prefork (server, mu_tcp_wrapper_prefork);
  mu_m_server_set_mode (server, MODE_INTERACTIVE);
  mu_m_server_set_max_children (server, 20);
  /* FIXME mu_m_server_set_pidfile (); */
  mu_m_server_set_default_port (server, 110);
  mu_m_server_set_timeout (server, 600);
    
  if (mu_app_init (&argp, pop3d_argp_capa, pop3d_cfg_param, 
		   argc, argv, 0, NULL, server))
    exit (1);

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
      gr = getgrnam ("mail");
      if (gr == NULL)
	{
	  perror (_("Error getting mail group"));
	  exit (EXIT_FAILURE);
	}

      if (setgid (gr->gr_gid) == -1)
	{
	  perror (_("Error setting mail group"));
	  exit (EXIT_FAILURE);
	}
    }

  /* Set the signal handlers.  */
  signal (SIGINT, pop3d_signal);
  signal (SIGQUIT, pop3d_signal);
  signal (SIGILL, pop3d_signal);
  signal (SIGBUS, pop3d_signal);
  signal (SIGFPE, pop3d_signal);
  signal (SIGSEGV, pop3d_signal);
  signal (SIGTERM, pop3d_signal);
  signal (SIGSTOP, pop3d_signal);
  signal (SIGPIPE, pop3d_signal);
  signal (SIGABRT, pop3d_signal);

  /* Set up for syslog.  */
  openlog ("gnu-pop3d", LOG_PID, log_facility);
  /* Redirect any stdout error from the library to syslog, they
     should not go to the client.  */
  {
    mu_debug_t debug;

    mu_diag_get_debug (&debug);
    mu_debug_set_print (debug, mu_diag_syslog_printer, NULL);
    
    mu_debug_default_printer = mu_debug_syslog_printer;
  }
  
  umask (S_IROTH | S_IWOTH | S_IXOTH);	/* 007 */

  /* Check TLS environment, i.e. cert and key files */
#ifdef WITH_TLS
  tls_available = mu_check_tls_environment ();
  if (tls_available)
    tls_available = mu_init_tls_libs ();
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
      status = pop3d_mainloop (fileno (stdin), stdin, stdout);
    }
  
  if (status)
    mu_error (_("Main loop status: %s"), mu_strerror (status));	  
  /* Close the syslog connection and exit.  */
  closelog ();
  return (OK != status);
}


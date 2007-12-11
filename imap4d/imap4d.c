/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 
   2005, 2006, 2007 Free Software Foundation, Inc.

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

#include "imap4d.h"
#ifdef WITH_GSASL
# include <mailutils/gsasl.h>
#endif
#include "mailutils/libargp.h"
#include "tcpwrap.h"

mu_mailbox_t mbox;
char *homedir;
int state = STATE_NONAUTH;
struct mu_auth_data *auth_data;

struct mu_gocs_daemon default_gocs_daemon = {
  MODE_INTERACTIVE,		/* Start in interactive (inetd) mode */
  20,				/* Default maximum number of children */
  143,				/* Standard IMAP4 port */
  1800,				/* RFC2060: 30 minutes. */
  0,				/* No transcript by default */
  NULL                          /* No PID file by default */
};

int login_disabled;             /* Disable LOGIN command */
int tls_required;               /* Require STARTTLS */
int create_home_dir;            /* Create home directory if it does not
				   exist */
int home_dir_mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

enum imap4d_preauth preauth_mode;
char *preauth_program;
int preauth_only;
int ident_port;
char *ident_keyfile;
int ident_encrypt_only;

/* Number of child processes.  */
size_t children;

const char *program_version = "imap4d (" PACKAGE_STRING ")";
static char doc[] = N_("GNU imap4d -- the IMAP4D daemon");

#define ARG_LOGIN_DISABLED  1
#define ARG_TLS_REQUIRED    2
#define ARG_CREATE_HOME_DIR 3

static struct argp_option options[] = {
  {"other-namespace", 'O', N_("PATHLIST"), OPTION_HIDDEN,
   N_("Set the `other' namespace"), 0},
  {"shared-namespace", 'S', N_("PATHLIST"), OPTION_HIDDEN,
   N_("Set the `shared' namespace"), 0},
  {"login-disabled", ARG_LOGIN_DISABLED, NULL, OPTION_HIDDEN,
   N_("Disable LOGIN command")},
  {"create-home-dir", ARG_CREATE_HOME_DIR, N_("MODE"),
   OPTION_ARG_OPTIONAL|OPTION_HIDDEN,
   N_("Create home directory, if it does not exist")},
#ifdef WITH_TLS
  {"tls-required", ARG_TLS_REQUIRED, NULL, OPTION_HIDDEN,
   N_("Always require STARTTLS before entering authentication phase")},
#endif
  {NULL, 0, NULL, 0, NULL, 0}
};


static error_t imap4d_parse_opt (int key, char *arg,
				 struct argp_state *state);

static struct argp argp = {
  options,
  imap4d_parse_opt,
  NULL,
  doc,
  NULL,
  NULL, NULL
};

static const char *imap4d_capa[] = {
  "daemon",
  "auth",
  "common",
  "debug",
  "mailbox",
  "locking",
  "logging",
  "license",
  NULL
};

static void imap4d_daemon_init (void);
static void imap4d_daemon (unsigned int, unsigned int);
static int imap4d_mainloop (int, FILE *, FILE *);

static error_t
imap4d_parse_opt (int key, char *arg, struct argp_state *state)
{
  static struct mu_argp_node_list lst;

  switch (key)
    {
    case 'O':
      mu_argp_node_list_new (&lst, "other-namespace", arg);
      break;

    case 'S':
      mu_argp_node_list_new (&lst, "shared-namespace", arg);
      break;

    case ARG_LOGIN_DISABLED:
      mu_argp_node_list_new (&lst, "login-disabled", "yes");
      break;

    case ARG_CREATE_HOME_DIR:
      mu_argp_node_list_new (&lst, "create-home-dir", "yes");
      if (arg)
	mu_argp_node_list_new (&lst, "home-dir-mode", arg);
      break;
	
#ifdef WITH_TLS
    case ARG_TLS_REQUIRED:
      mu_argp_node_list_new (&lst, "tls-required", "yes");
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

static int
cb_other (mu_debug_t debug, void *data, char *arg)
{
  set_namespace (NS_OTHER, arg);
  return 0;
}

static int
cb_shared (mu_debug_t debug, void *data, char *arg)
{
  set_namespace (NS_SHARED, arg);
  return 0;
}

static int
cb_mode (mu_debug_t debug, void *data, char *arg)
{
  char *p;
  home_dir_mode = strtoul (arg, &p, 8);
  if (p[0] || (home_dir_mode & ~0777))
    mu_cfg_format_error (debug, MU_DEBUG_ERROR, 
                         _("Invalid mode specification: %s"), arg);
  return 0;
}

int
parse_preauth_scheme (mu_debug_t debug, const char *scheme, mu_url_t url)
{
  int rc = 0;
  if (strcmp (scheme, "stdio") == 0)
    preauth_mode = preauth_stdio;
  else if (strcmp (scheme, "prog") == 0)
    {
      char *path;
      rc = mu_url_aget_path (url, &path);
      if (rc)
	{
	  mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			       _("URL error: cannot get path: %s"),
			       mu_strerror (rc));
	  return 1;
	}
      preauth_program = path;
      preauth_mode = preauth_prog;
    }
  else if (strcmp (scheme, "ident") == 0)
    {
      struct servent *sp;
      long n;
      if (url && mu_url_get_port (url, &n) == 0)
	ident_port = (short) n;
      else if ((sp = getservbyname ("auth", "tcp")))
	ident_port = ntohs (sp->s_port);
      else
	ident_port = 113;
      preauth_mode = preauth_ident;
    }
  else
    {
      mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("unknown preauth scheme"));
      rc = 1;
    }

  return rc;
}
      
/* preauth prog:///usr/sbin/progname
   preauth ident[://:port]
   preauth stdio
*/
static int
cb_preauth (mu_debug_t debug, void *data, char *arg)
{
  if (strcmp (arg, "stdio") == 0)
    preauth_mode = preauth_stdio;
  else if (strcmp (arg, "ident") == 0)
    return parse_preauth_scheme (debug, arg, NULL);
  else if (arg[0] == '/')
    {
      preauth_program = xstrdup (arg);
      preauth_mode = preauth_prog;
    }
  else
    {
      mu_url_t url;
      char *scheme;
      int rc = mu_url_create (&url, arg);

      if (rc)
	{
	  mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			       _("cannot create URL: %s"), mu_strerror (rc));
	  return 1;
	}
      rc = mu_url_parse (url);
      if (rc)
	{
	  mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			       "%s: %s", arg, mu_strerror (rc));
	  return 1;
	}

      rc = mu_url_aget_scheme (url, &scheme);
      if (rc)
	{
	  mu_url_destroy (&url);
	  mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			       _("URL error: %s"), mu_strerror (rc));
	  return 1;
	}

      rc = parse_preauth_scheme (debug, scheme, url);
      mu_url_destroy (&url);
      free (scheme);
      return rc;
    }
  return 0;
}
	
static struct mu_cfg_param imap4d_cfg_param[] = {
  { "other-namespace", mu_cfg_callback, NULL, 0, cb_other, 
    N_("Set other users' namespace.  Argument is a colon-separated list "
       "of directories comprising the namespace.") },
  { "shared-namespace", mu_cfg_callback, NULL, 0, cb_shared,
    N_("Set shared namespace.  Argument is a colon-separated list "
       "of directories comprising the namespace.") },
  { "login-disabled", mu_cfg_int, &login_disabled, 0, NULL,
    N_("Disable LOGIN command.") },
  { "create-home-dir", mu_cfg_bool, &create_home_dir, 0, NULL,
    N_("If true, create non-existing user home directories.") },
  { "home-dir-mode", mu_cfg_callback, NULL, 0, cb_mode,
    N_("File mode for creating user home directories (octal)."),
    N_("mode") },
  { "tls-required", mu_cfg_int, &tls_required, 0, NULL,
    N_("Always require STARTTLS before entering authentication phase.") },
  { "preauth", mu_cfg_callback, NULL, 0, cb_preauth,
    N_("Configure PREAUTH mode.  MODE is one of:\n"
       "  prog:///<full-program-name: string>\n"
       "  ident[://:<port: string-or-number>]\n"
       "  stdio"),
    N_("MODE") },
  { "preauth-only", mu_cfg_bool, &preauth_only, 0, NULL,
    N_("Use only preauth mode.  If unable to setup it, disconnect "
       "immediately.") },
  { "ident-keyfile", mu_cfg_string, &ident_keyfile, 0, NULL,
    N_("Name of DES keyfile for decoding ecrypted ident responses.") },
  { "ident-entrypt-only", mu_cfg_bool, &ident_encrypt_only, 0, NULL,
    N_("Use only encrypted ident responses.") },
  TCP_WRAPPERS_CONFIG
  { NULL }
};
    
int
main (int argc, char **argv)
{
  struct group *gr;
  int status = EXIT_SUCCESS;

  /* Native Language Support */
  mu_init_nls ();

  state = STATE_NONAUTH;	/* Starting state in non-auth.  */

  MU_AUTH_REGISTER_ALL_MODULES ();
  imap4d_capability_init ();
  mu_gocs_daemon = default_gocs_daemon;
#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif /* WITH_TLS */
#ifdef WITH_GSASL
  mu_gocs_register ("gsasl", mu_gsasl_module_init);
#endif
  mu_argp_init (program_version, NULL);
  if (mu_app_init (&argp, imap4d_capa, imap4d_cfg_param, 
		   argc, argv, 0, NULL, NULL))
    exit (1);

  if (login_disabled)
    imap4d_capability_add (IMAP_CAPA_LOGINDISABLED);
#ifdef WITH_TLS
  if (tls_required)
    imap4d_capability_add (IMAP_CAPA_XTLSREQUIRED);
#endif
  
  auth_gssapi_init ();
  auth_gsasl_init ();

#ifdef USE_LIBPAM
  if (!mu_pam_service)
    mu_pam_service = "gnu-imap4d";
#endif

  if (mu_gocs_daemon.mode == MODE_DAEMON)
    {
      /* Normal operation: */
      /* First we want our group to be mail so we can access the spool.  */
      gr = getgrnam ("mail");
      if (gr == NULL)
	{
	  perror (_("Error getting mail group"));
	  exit (1);
	}

      if (setgid (gr->gr_gid) == -1)
	{
	  perror (_("Error setting mail group"));
	  exit (1);
	}
    }

  /* Register the desired formats. */
  mu_register_local_mbox_formats ();
  
  /* Set the signal handlers.  */
  signal (SIGINT, imap4d_signal);
  signal (SIGQUIT, imap4d_signal);
  signal (SIGILL, imap4d_signal);
  signal (SIGBUS, imap4d_signal);
  signal (SIGFPE, imap4d_signal);
  signal (SIGSEGV, imap4d_signal);
  signal (SIGTERM, imap4d_signal);
  signal (SIGSTOP, imap4d_signal);
  signal (SIGPIPE, imap4d_signal);
  /*signal (SIGPIPE, SIG_IGN); */
  signal (SIGABRT, imap4d_signal);

  if (mu_gocs_daemon.mode == MODE_DAEMON)
    imap4d_daemon_init ();
  else
    {
      /* Make sure we are in the root.  */
      chdir ("/");
    }

  /* Set up for syslog.  */
  openlog ("gnu-imap4d", LOG_PID, log_facility);

  /* Redirect any stdout error from the library to syslog, they
     should not go to the client.  */
  {
    mu_debug_t debug;

    mu_diag_get_debug (&debug);
    mu_debug_set_print (debug, mu_diag_syslog_printer, NULL);
  }

  umask (S_IROTH | S_IWOTH | S_IXOTH);	/* 007 */

  if (mu_gocs_daemon.pidfile)
    {
      mu_daemon_create_pidfile (mu_gocs_daemon.pidfile);
    }

  /* Check TLS environment, i.e. cert and key files */
#ifdef WITH_TLS
  starttls_init ();
#endif /* WITH_TLS */

  /* Actually run the daemon.  */
  if (mu_gocs_daemon.mode == MODE_DAEMON)
    imap4d_daemon (mu_gocs_daemon.maxchildren, mu_gocs_daemon.port);
  /* exit (0) -- no way out of daemon except a signal.  */
  else
    status = imap4d_mainloop (fileno (stdin), stdin, stdout);

  /* Close the syslog connection and exit.  */
  closelog ();

  return status;
}

int
imap4d_session_setup0 ()
{
  homedir = mu_normalize_path (strdup (auth_data->dir), "/");
  if (imap4d_check_home_dir (homedir, auth_data->uid, auth_data->gid))
    return 1;
  
  if (auth_data->change_uid)
    setuid (auth_data->uid);

  util_chdir (homedir);
  namespace_init (homedir);
  mu_diag_output (MU_DIAG_INFO,
		  _("User `%s' logged in (source: %s)"), auth_data->name,
		  auth_data->source);
  return 0;
}

int
imap4d_session_setup (char *username)
{
  auth_data = mu_get_auth_by_name (username);
  if (auth_data == NULL)
    {
      mu_diag_output (MU_DIAG_INFO, _("User `%s': nonexistent"), username);
      return 1;
    }
  return imap4d_session_setup0 ();
}

int
get_client_address (int fd, struct sockaddr_in *pcs)
{
  int len = sizeof *pcs;

  if (getpeername (fd, (struct sockaddr *) pcs, &len) < 0)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("Cannot obtain IP address of client: %s"),
		      strerror (errno));
      return 1;
    }

  mu_diag_output (MU_DIAG_INFO, _("Connect from %s"), 
		  inet_ntoa (pcs->sin_addr));
  return 0;
}

static int
imap4d_mainloop (int fd, FILE *infile, FILE *outfile)
{
  char *text;
  struct sockaddr_in cs;
  int debug_mode = isatty (fd);

  mu_diag_output (MU_DIAG_INFO, _("Incoming connection opened"));
  if (!debug_mode)
    {
      if (get_client_address (fd, &cs) == 0) 
	{
	  if (!mu_tcpwrapper_access (fd))
	    {
	      mu_error (_("Access from %s blocked."), inet_ntoa (cs.sin_addr));
	      return 1;
	    }
	}
      else if (mu_tcp_wrapper_enable)
	{
	  mu_error (_("Rejecting connection from unknown address"));
	  return 1;
	}
    }
  
  /* Reset hup to exit. */
  signal (SIGHUP, imap4d_signal);
  /* Timeout alarm. */
  signal (SIGALRM, imap4d_signal);

  util_setio (infile, outfile);

  if (imap4d_preauth_setup (fd) == 0)
    {
      if (debug_mode)
	{
	  mu_diag_output (MU_DIAG_INFO, _("Started in debugging mode"));
	  text = "IMAP4rev1 Debugging mode";
	}
      else
	text = "IMAP4rev1";
    }
  else
    {
      util_flush_output ();
      return EXIT_SUCCESS;
    }

  /* Greetings.  */
  util_out ((state == STATE_AUTH) ? RESP_PREAUTH : RESP_OK, "%s", text);
  util_flush_output ();

  while (1)
    {
      char *cmd = imap4d_readline ();
      /* check for updates */
      imap4d_sync ();
      util_do_command (cmd);
      imap4d_sync ();
      free (cmd);
      util_flush_output ();
    }

  return EXIT_SUCCESS;
}

/* Sets things up for daemon mode.  */
static void
imap4d_daemon_init (void)
{
  extern int daemon (int, int);

  /* Become a daemon. Take care to close inherited fds and to hold
     first three one, in, out, err   */
  if (daemon (0, 0) < 0)
    {
      perror (_("could not become daemon"));
      exit (1);
    }

  /* SIGCHLD is not ignore but rather use to do some simple load balancing.  */
#ifdef HAVE_SIGACTION
  {
    struct sigaction act;
    act.sa_handler = imap4d_sigchld;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGCHLD, &act, NULL);
  }
#else
  signal (SIGCHLD, imap4d_sigchld);
#endif
}

/* Runs GNU imap4d in standalone daemon mode. This opens and binds to a port
   (default 143) then executes a imap4d_mainloop() upon accepting a connection.
   It starts maxchildren child processes to listen to and accept socket
   connections.  */
static void
imap4d_daemon (unsigned int maxchildren, unsigned int port)
{
  struct sockaddr_in server, client;
  pid_t pid;
  int listenfd, connfd;
  size_t size;

  listenfd = socket (PF_INET, SOCK_STREAM, 0);
  if (listenfd == -1)
    {
      mu_diag_output (MU_DIAG_ERROR, "socket: %s", strerror (errno));
      exit (1);
    }
  size = 1;	      /* Use size here to avoid making a new variable.  */
  setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &size, sizeof (size));
  size = sizeof (server);
  memset (&server, 0, size);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (port);

  if (bind (listenfd, (struct sockaddr *) &server, size) == -1)
    {
      mu_diag_output (MU_DIAG_ERROR, "bind: %s", strerror (errno));
      exit (1);
    }

  if (listen (listenfd, 128) == -1)
    {
      mu_diag_output (MU_DIAG_ERROR, "listen: %s", strerror (errno));
      exit (1);
    }

  mu_diag_output (MU_DIAG_INFO, _("GNU imap4d started"));

  for (;;)
    {
      if (children > maxchildren)
	{
	  mu_diag_output (MU_DIAG_ERROR, _("Too many children (%s)"),
		  mu_umaxtostr (0, children));
	  pause ();
	  continue;
	}
      connfd = accept (listenfd, (struct sockaddr *) &client,
		       (socklen_t *) & size);
      if (connfd == -1)
	{
	  if (errno == EINTR)
	    continue;
	  mu_diag_output (MU_DIAG_ERROR, "accept: %s", strerror (errno));
	  exit (1);
	}

      pid = fork ();
      if (pid == -1)
	mu_diag_output (MU_DIAG_ERROR, "fork: %s", strerror (errno));
      else if (pid == 0)	/* Child.  */
	{
	  int status;
	  
	  close (listenfd);

	  status = imap4d_mainloop (connfd,
				    fdopen (connfd, "r"),
				    fdopen (connfd, "w"));
	    
	  close (connfd);
	  closelog ();
	  exit (status);
	}
      else
	{
	  ++children;
	}
      close (connfd);
    }
}

int
imap4d_check_home_dir (const char *dir, uid_t uid, gid_t gid)
{
  struct stat st;

  if (stat (homedir, &st))
    {
      if (errno == ENOENT && create_home_dir)
	{
	  mode_t mode = umask (0);
	  int rc = mkdir (homedir, home_dir_mode);
	  umask (mode);
	  if (rc)
	    {
	      mu_error ("Cannot create home directory `%s': %s",
			homedir, mu_strerror (errno));
	      return 1;
	    }
	  if (chown (homedir, uid, gid))
	    {
	      mu_error ("Cannot set owner for home directory `%s': %s",
			homedir, mu_strerror (errno));
	      return 1;
	    }
	}
    }
  
  return 0;
}


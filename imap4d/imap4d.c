/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 
   2005  Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
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

mailbox_t mbox;
char *homedir;
int state = STATE_NONAUTH;
int debug_mode = 0;
struct mu_auth_data *auth_data;

struct daemon_param daemon_param = {
  MODE_INTERACTIVE,		/* Start in interactive (inetd) mode */
  20,				/* Default maximum number of children */
  143,				/* Standard IMAP4 port */
  1800,				/* RFC2060: 30 minutes. */
  0,				/* No transcript by default */
  NULL                          /* No PID file by default */
};

int login_disabled;             /* Disable LOGIN command */
int tls_required;               /* Require STARTTLS */

/* Number of child processes.  */
size_t children;

const char *program_version = "imap4d (" PACKAGE_STRING ")";
static char doc[] = N_("GNU imap4d -- the IMAP4D daemon");

#define ARG_LOGIN_DISABLED 1
#define ARG_TLS_REQUIRED   2

static struct argp_option options[] = {
  {"other-namespace", 'O', N_("PATHLIST"), 0,
   N_("Set the `other' namespace"), 0},
  {"shared-namespace", 'S', N_("PATHLIST"), 0,
   N_("Set the `shared' namespace"), 0},
  {"login-disabled", ARG_LOGIN_DISABLED, NULL, 0,
   N_("Disable LOGIN command")},
#ifdef WITH_TLS
  {"tls-required", ARG_TLS_REQUIRED, NULL, 0,
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
#ifdef WITH_TLS
  "tls",
#endif /* WITH_TLS */
#ifdef WITH_GSASL
  "gsasl",
#endif
  "common",
  "mailbox",
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
  switch (key)
    {
    case ARGP_KEY_INIT:
      state->child_inputs[0] = state->input;
      break;

    case 'O':
      set_namespace (NS_OTHER, arg);
      break;

    case 'S':
      set_namespace (NS_SHARED, arg);
      break;

    case ARG_LOGIN_DISABLED:
      login_disabled = 1;
      imap4d_capability_add (IMAP_CAPA_LOGINDISABLED);
      break;

#ifdef WITH_TLS
    case ARG_TLS_REQUIRED:
      tls_required = 1;
      imap4d_capability_add (IMAP_CAPA_XTLSREQUIRED);
      break;
#endif
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

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

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_argp ();
#endif /* WITH_TLS */
#ifdef WITH_GSASL
  mu_gsasl_init_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, imap4d_capa, NULL, &daemon_param);

  auth_gssapi_init ();
  auth_gsasl_init ();
  
#ifdef USE_LIBPAM
  if (!pam_service)
    pam_service = "gnu-imap4d";
#endif

  if (daemon_param.mode == MODE_INTERACTIVE && isatty (0))
    {
      /* If input is a tty, switch to debug mode */
      debug_mode = 1;
    }
  else
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

  if (daemon_param.mode == MODE_DAEMON)
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
  mu_error_set_print (mu_syslog_error_printer);

  umask (S_IROTH | S_IWOTH | S_IXOTH);	/* 007 */

  if (daemon_param.pidfile)
    {
      mu_daemon_create_pidfile (daemon_param.pidfile);
    }

  /* Check TLS environment, i.e. cert and key files */
#ifdef WITH_TLS
  starttls_init ();
#endif /* WITH_TLS */

  /* Actually run the daemon.  */
  if (daemon_param.mode == MODE_DAEMON)
    imap4d_daemon (daemon_param.maxchildren, daemon_param.port);
  /* exit (0) -- no way out of daemon except a signal.  */
  else
    status = imap4d_mainloop (fileno (stdin), stdin, stdout);

  /* Close the syslog connection and exit.  */
  closelog ();

  return status;
}

static int
imap4d_mainloop (int fd, FILE *infile, FILE *outfile)
{
  char *text;

  /* Reset hup to exit.  */
  signal (SIGHUP, imap4d_signal);
  /* Timeout alarm.  */
  signal (SIGALRM, imap4d_signal);

  util_setio (infile, outfile);

  /* log information on the connecting client */
  if (!debug_mode)
    {
      struct sockaddr_in cs;
      int len = sizeof cs;

      syslog (LOG_INFO, _("Incoming connection opened"));
      if (getpeername (fd, (struct sockaddr *) &cs, &len) < 0)
	syslog (LOG_ERR, _("Cannot obtain IP address of client: %s"),
		strerror (errno));
      else
	syslog (LOG_INFO, _("Connect from %s"), inet_ntoa (cs.sin_addr));
      text = "IMAP4rev1";
    }
  else
    {
      syslog (LOG_INFO, _("Started in debugging mode"));
      text = "IMAP4rev1 Debugging mode";
    }

  /* Greetings.  */
  util_out (RESP_OK, text);
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
      syslog (LOG_ERR, "socket: %s", strerror (errno));
      exit (1);
    }
  size = 1;			/* Use size here to avoid making a new variable.  */
  setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &size, sizeof (size));
  size = sizeof (server);
  memset (&server, 0, size);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (port);

  if (bind (listenfd, (struct sockaddr *) &server, size) == -1)
    {
      syslog (LOG_ERR, "bind: %s", strerror (errno));
      exit (1);
    }

  if (listen (listenfd, 128) == -1)
    {
      syslog (LOG_ERR, "listen: %s", strerror (errno));
      exit (1);
    }

  syslog (LOG_INFO, _("GNU imap4d started"));

  for (;;)
    {
      if (children > maxchildren)
	{
	  syslog (LOG_ERR, _("Too many children (%lu)"),
		  (unsigned long) children);
	  pause ();
	  continue;
	}
      connfd = accept (listenfd, (struct sockaddr *) &client,
		       (socklen_t *) & size);
      if (connfd == -1)
	{
	  if (errno == EINTR)
	    continue;
	  syslog (LOG_ERR, "accept: %s", strerror (errno));
	  exit (1);
	}

      pid = fork ();
      if (pid == -1)
	syslog (LOG_ERR, "fork: %s", strerror (errno));
      else if (pid == 0)	/* Child.  */
	{
	  int status;
	  close (listenfd);
	  status = imap4d_mainloop (connfd,
				    fdopen (connfd, "r"),
				    fdopen (connfd, "w"));
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

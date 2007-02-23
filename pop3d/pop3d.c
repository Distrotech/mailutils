/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 
   2005, 2007 Free Software Foundation, Inc.

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

#include "pop3d.h"

mu_mailbox_t mbox;
int state;
char *username;
char *md5shared;

struct daemon_param daemon_param = {
  MODE_INTERACTIVE,     /* Start in interactive (inetd) mode */
  20,                   /* Default maximum number of children */
  110,                  /* Standard POP3 port */
  600,                  /* Idle timeout */
  0,                    /* No transcript by default */
  NULL                  /* No PID file by default */
};

int debug_mode;

#ifdef WITH_TLS
int tls_available;
int tls_done;
#endif /* WITH_TLS */

int initial_state = AUTHORIZATION; 

/* Number of child processes.  */
size_t children;
/* Should all the messages be undeleted on startup */
int undelete_on_startup;
#ifdef ENABLE_LOGIN_DELAY
/* Minimum allowed delay between two successive logins */
time_t login_delay = 0;
char *login_stat_file = LOGIN_STAT_FILE;
#endif

time_t expire = EXPIRE_NEVER; /* Expire messages after this number of days */
int expire_on_exit = 0;       /* Delete expired messages on exit */

static int pop3d_mainloop       (int fd, FILE *, FILE *);
static void pop3d_daemon_init   (void);
static void pop3d_daemon        (unsigned int, unsigned int);
static error_t pop3d_parse_opt  (int key, char *arg, struct argp_state *astate);
static void pop3d_log_connection (int fd);

const char *program_version = "pop3d (" PACKAGE_STRING ")";
static char doc[] = N_("GNU pop3d -- the POP3 daemon");

#define OPT_LOGIN_DELAY     257
#define OPT_STAT_FILE       258
#define OPT_EXPIRE          259
#define OPT_EXPIRE_ON_EXIT  260
#define OPT_TLS_REQUIRED    261
#define OPT_BULLETIN_SOURCE 262
#define OPT_BULLETIN_DB     263

static struct argp_option options[] = {
#define GRP 0
  { NULL, 0, NULL, 0,
    N_("General options"), GRP },
  {"undelete", 'u', NULL, 0,
   N_("Undelete all messages on startup"), GRP+1},
  {"expire", OPT_EXPIRE, N_("DAYS"), 0,
   N_("Expire read messages after the given number of days"), GRP+1},
  {"delete-expired", OPT_EXPIRE_ON_EXIT, NULL, 0,
   N_("Delete expired messages upon closing the mailbox"), GRP+1},
#ifdef WITH_TLS
  {"tls-required", OPT_TLS_REQUIRED, NULL, 0,
   N_("Always require STLS before entering authentication phase")},
#endif
#undef GRP
  
#define GRP 10
#ifdef ENABLE_LOGIN_DELAY
  { NULL, 0, NULL, 0,
    N_("Login delay control"), GRP },
  {"login-delay", OPT_LOGIN_DELAY, N_("SECONDS"), 0,
   N_("Allowed delay between the two successive logins"), GRP+1},
  {"stat-file", OPT_STAT_FILE, N_("FILENAME"), 0,
   N_("Name of login statistics file"), GRP+1},
#endif
  
#undef GRP

#define GRP 20
  { NULL, 0, NULL, 0,
    N_("Bulletin control"), GRP },
  { "bulletin-source", OPT_BULLETIN_SOURCE, N_("MBOX"), 0,
    N_("Set source mailbox to get bulletins from"), GRP+1 },
#ifdef USE_DBM
  { "bulletin-db", OPT_BULLETIN_DB, N_("FILE"), 0,
    N_("Set the bulletin database file name"), GRP+1 },
#endif
#undef GRP
  
  {NULL, 0, NULL, 0, NULL, 0}
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
  "daemon",
  "auth",
#ifdef WITH_TLS
  "tls",
#endif /* WITH_TLS */
  "common",
  "mailbox",
  "logging",
  "license",
  NULL
};

static error_t
pop3d_parse_opt (int key, char *arg, struct argp_state *astate)
{
  char *p;
  
  switch (key)
    {
    case ARGP_KEY_INIT:
      astate->child_inputs[0] = astate->input;
      break;

    case 'u':
      undelete_on_startup = 1;
      break;

#ifdef ENABLE_LOGIN_DELAY
    case OPT_LOGIN_DELAY:
      login_delay = strtoul (arg, &p, 10);
      if (*p)
	{
	  argp_error (astate, _("Invalid number"));
	  exit (1);
	}
      break;

    case OPT_STAT_FILE:
      login_stat_file = arg;
      break;
#endif  
 
    case OPT_EXPIRE:
      expire = strtoul (arg, &p, 10);
      if (*p)
	{
	  argp_error (astate, _("Invalid number"));
	  exit (1);
	}
      break;

    case OPT_EXPIRE_ON_EXIT:
      expire_on_exit = 1;
      break;

#ifdef WITH_TLS
    case OPT_TLS_REQUIRED:
      initial_state = INITIAL;
      break;
#endif

    case OPT_BULLETIN_SOURCE:
      set_bulletin_source (arg);
      break;
      
#ifdef USE_DBM
    case OPT_BULLETIN_DB:
      set_bulletin_db (arg);
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
  int status = OK;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
  MU_AUTH_REGISTER_ALL_MODULES();
  /* Register the desired formats.  */
  mu_register_local_mbox_formats ();

#ifdef WITH_TLS
  mu_tls_init_argp ();
#endif /* WITH_TLS */
  mu_argp_parse (&argp, &argc, &argv, 0, pop3d_argp_capa, NULL, &daemon_param);

#ifdef USE_LIBPAM
  if (!pam_service)
    pam_service = "gnu-pop3d";
#endif

  if (daemon_param.mode == MODE_INTERACTIVE && isatty (0))
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

  if (daemon_param.mode == MODE_DAEMON)
    pop3d_daemon_init ();
  else
    {
      /* Make sure we are in the root directory.  */
      chdir ("/");
    }

  /* Set up for syslog.  */
  openlog ("gnu-pop3d", LOG_PID, log_facility);
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
  tls_available = mu_check_tls_environment ();
  if (tls_available)
    tls_available = mu_init_tls_libs ();
#endif /* WITH_TLS */

  /* Actually run the daemon.  */
  if (daemon_param.mode == MODE_DAEMON)
    pop3d_daemon (daemon_param.maxchildren, daemon_param.port);
  /* exit (EXIT_SUCCESS) -- no way out of daemon except a signal.  */
  else
    status = pop3d_mainloop (fileno (stdin), stdin, stdout);

  /* Close the syslog connection and exit.  */
  closelog ();
  return (OK != status);
}

/* Sets things up for daemon mode.  */
static void
pop3d_daemon_init (void)
{
  extern int daemon (int, int);

  /* Become a daemon. Take care to close inherited fds and to hold
     first three one, in, out, err   */
  if (daemon (0, 0) < 0)
    {
      perror (_("Failed to become a daemon:"));
      exit (EXIT_FAILURE);
    }

  /* SIGCHLD is not ignore but rather use to do some simple load balancing.  */
#ifdef HAVE_SIGACTION
  {
    struct sigaction act;
    act.sa_handler = pop3d_sigchld;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGCHLD, &act, NULL);
  }
#else
  signal (SIGCHLD, pop3d_sigchld);
#endif
}

void
pop3d_log_connection (int fd)
{
  syslog (LOG_INFO, _("Incoming connection opened"));

  /* log information on the connecting client */
  if (debug_mode)
    {
      syslog (LOG_INFO, _("Started in debugging mode"));
    }
  else
    {
      struct sockaddr_in cs;
      int len = sizeof cs;
      if (getpeername (fd, (struct sockaddr*)&cs, &len) < 0)
	syslog (LOG_ERR, _("Cannot obtain IP address of client: %s"),
		strerror (errno));
      else
	syslog (LOG_INFO, _("connect from %s"), inet_ntoa (cs.sin_addr));
    }
}

/* The main part of the daemon. This function reads input from the client and
   executes the proper functions. Also handles the bulk of error reporting.
   Arguments:
      fd        --  socket descriptor (for diagnostics)
      infile    --  input stream
      outfile   --  output stream */
static int
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

  pop3d_log_connection (fd);

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
	      syslog (LOG_ERR, _("Session terminated"));
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

/* Runs GNU POP3 in standalone daemon mode. This opens and binds to a port
   (default 110) then executes a pop3d_mainloop() upon accepting a connection.
   It starts maxchildren child processes to listen to and accept socket
   connections.  */
static void
pop3d_daemon (unsigned int maxchildren, unsigned int port)
{
  struct sockaddr_in server, client;
  pid_t pid;
  int listenfd, connfd;
  size_t size;

  listenfd = socket (PF_INET, SOCK_STREAM, 0);
  if (listenfd == -1)
    {
      syslog (LOG_ERR, "socket: %s", strerror(errno));
      exit (EXIT_FAILURE);
    }
  size = 1; /* Use size here to avoid making a new variable.  */
  setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &size, sizeof(size));
  size = sizeof (server);
  memset (&server, 0, size);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (port);

  if (bind (listenfd, (struct sockaddr *)&server, size) == -1)
    {
      syslog (LOG_ERR, "bind: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (listen (listenfd, 128) == -1)
    {
      syslog (LOG_ERR, "listen: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  syslog (LOG_INFO, _("GNU pop3d started"));

  for (;;)
    {
      process_cleanup ();
      if (children > maxchildren)
        {
	  syslog (LOG_ERR, _("too many children (%s)"),
		  mu_umaxtostr (0, children));
          pause ();
          continue;
        }
      connfd = accept (listenfd, (struct sockaddr *)&client,
		       (socklen_t *) &size);
      if (connfd == -1)
        {
          if (errno == EINTR)
	    continue;
          syslog (LOG_ERR, "accept: %s", strerror (errno));
          continue;
          /*exit (EXIT_FAILURE);*/
        }

      pid = fork ();
      if (pid == -1)
	syslog(LOG_ERR, "fork: %s", strerror (errno));
      else if (pid == 0) /* Child.  */
        {
	  int status;
	  
	  close (listenfd);
          status = pop3d_mainloop (connfd,
				   fdopen (connfd, "r"), fdopen (connfd, "w"));
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


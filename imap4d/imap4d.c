/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"
#ifdef HAVE_MYSQL
# include "../MySql/MySql.h"
#endif

FILE *ifile;
FILE *ofile;
mailbox_t mbox;
char *homedir;
int state = STATE_NONAUTH;

struct daemon_param daemon_param = {
  MODE_INTERACTIVE,     /* Start in interactive (inetd) mode */
  20,                   /* Default maximum number of children */
  143,                  /* Standard IMAP4 port */
  1800                  /* RFC2060: 30 minutes. */ 
};

/* Number of child processes.  */
volatile size_t children;

const char *argp_program_version = "imap4d (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU imap4d -- the IMAP4D daemon";

static struct argp_option options[] = 
{
  {"other-namespace", 'O', "PATHLIST", 0,
   "set the `other' namespace", 0},
  {"shared-namespace", 'S', "PATHLIST", 0,
   "set the `shared' namespace", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t imap4d_parse_opt (int key, char *arg, struct argp_state *state);

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
  "mailbox",
  "logging",
  "licence",
  NULL
};

static int imap4d_mainloop      __P ((int, int));
static void imap4d_daemon_init  __P ((void));
static void imap4d_daemon       __P ((unsigned int, unsigned int));
static int imap4d_mainloop      __P ((int, int));

static error_t
imap4d_parse_opt (int key, char *arg, struct argp_state *state)
{
    switch (key)
      {
      case ARGP_KEY_INIT:
       	state->child_inputs[1] = state->input;
	break;
	
      case 'O':
	set_namespace (NS_OTHER, arg);
	break;
	
      case 'S':
	set_namespace (NS_SHARED, arg);
	break;
	
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

  state = STATE_NONAUTH; /* Starting state in non-auth.  */

  mu_argp_parse (&argp, &argc, &argv, 0, imap4d_capa, NULL, &daemon_param);

#ifdef USE_LIBPAM
  if (!pam_service)
    pam_service = "gnu-imap4d";
#endif

  /* First we want our group to be mail so we can access the spool.  */
  gr = getgrnam ("mail");
  if (gr == NULL)
    {
      perror ("Error getting mail group");
      exit (1);
    }

  if (setgid (gr->gr_gid) == -1)
    {
      perror ("Error setting mail group");
      exit (1);
    }

  /* Register the desire formats. We only need Mbox mail format.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record); 
    list_append (bookie, path_record);
  }

#ifdef HAVE_MYSQL
  mu_register_getpwnam (getMpwnam);
  mu_register_getpwuid (getMpwuid);
#endif
#ifdef USE_VIRTUAL_DOMAINS
  mu_register_getpwnam (getpwnam_virtual);
#endif

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

  umask (S_IROTH | S_IWOTH | S_IXOTH);  /* 007 */

  /* Actually run the daemon.  */
  if (daemon_param.mode == MODE_DAEMON)
    imap4d_daemon (daemon_param.maxchildren, daemon_param.port);
  /* exit (0) -- no way out of daemon except a signal.  */
  else
    status = imap4d_mainloop (fileno (stdin), fileno (stdout));

  /* Close the syslog connection and exit.  */
  closelog ();

  return status;
}

static int
imap4d_mainloop (int infile, int outfile)
{
  /* Reset hup to exit.  */
  signal (SIGHUP, imap4d_signal);
  /* Timeout alarm.  */
  signal (SIGALRM, imap4d_signal);

  ifile = fdopen (infile, "r");
  ofile = fdopen (outfile, "w");
  if (!ofile || !ifile)
    imap4d_bye (ERR_NO_OFILE);

  setvbuf(ofile, NULL, _IOLBF, 0);

  syslog (LOG_INFO, "Incoming connection opened");

  /* log information on the connecting client */
  {
    struct sockaddr_in cs;
    int len = sizeof cs;
    if (getpeername (infile, (struct sockaddr*)&cs, &len) < 0)
      syslog (LOG_ERR, "can't obtain IP address of client: %s",
              strerror (errno));
    else
      syslog (LOG_INFO, "connect from %s", inet_ntoa(cs.sin_addr));
  }

  /* Greetings.  */
  util_out (RESP_OK, "IMAP4rev1 GNU " PACKAGE " " VERSION);
  fflush (ofile);

  while (1)
    {
      char *cmd = imap4d_readline (ifile);
      /* check for updates */
      imap4d_sync ();
      util_do_command (cmd);
      imap4d_sync ();
      free (cmd);
      fflush (ofile);
    }

  closelog ();
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
      perror("fork failed:");
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

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1)
    {
      syslog (LOG_ERR, "socket: %s", strerror(errno));
      exit (1);
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
      exit (1);
    }

  if (listen (listenfd, 128) == -1)
    {
      syslog (LOG_ERR, "listen: %s", strerror (errno));
      exit (1);
    }

  for (;;)
    {
      if (children > maxchildren)
        {
          syslog (LOG_ERR, "too many children (%d)", children);
          pause ();
          continue;
        }
      connfd = accept (listenfd, (struct sockaddr *)&client, &size);
      if (connfd == -1)
        {
          if (errno == EINTR)
            continue;
          syslog (LOG_ERR, "accept: %s", strerror (errno));
          exit (1);
        }

      pid = fork ();
      if (pid == -1)
        syslog(LOG_ERR, "fork: %s", strerror (errno));
      else if (pid == 0) /* Child.  */
        {
          int status;
          close (listenfd);
          status = imap4d_mainloop (connfd, connfd);
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




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

FILE *ofile;
unsigned int timeout = 1800; /* RFC2060: 30 minutes, if enable.  */
mailbox_t mbox;
char *homedir;
int state = STATE_NONAUTH;

/* Number of child processes.  */
volatile size_t children;

static struct option long_options[] =
{
  {"daemon", optional_argument, 0, 'd'},
  {"help", no_argument, 0, 'h'},
  {"inetd", no_argument, 0, 'i'},
  {"port", required_argument, 0, 'p'},
  {"other-namespace", required_argument, 0, 'O'},
  {"shared-namespace", required_argument, 0, 'S'},
  {"timeout", required_argument, 0, 't'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

const char *short_options ="d::hip:t:vO:P:S:";

static int syslog_error_printer __P ((const char *fmt, va_list ap));
static int imap4d_mainloop      __P ((int, int));
static void imap4d_daemon_init  __P ((void));
static void imap4d_daemon       __P ((unsigned int, unsigned int));
static int imap4d_mainloop      __P ((int, int));
static void imap4d_usage       __P ((char *));

#ifndef DEFMAXCHILDREN
# define DEFMAXCHILDREN 20   /* Default maximum number of children */
#endif

int
main (int argc, char **argv)
{
  struct group *gr;
  static int mode = INTERACTIVE;
  size_t maxchildren = DEFMAXCHILDREN;
  int c = 0;
  int status = EXIT_SUCCESS;
  unsigned int port;

  port = 143;      /* Default IMAP4 port.  */
  timeout = 1800;  /* RFC2060: 30 minutes, if enable.  */
  state = STATE_NONAUTH; /* Starting state in non-auth.  */

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (c)
        {
        case 'd':
          mode = DAEMON;
          if (optarg)
            maxchildren = strtoul (optarg, NULL, 10);
          if (maxchildren == 0)
            maxchildren = DEFMAXCHILDREN;
          break;

        case 'h':
          imap4d_usage (argv[0]);
          break;

        case 'i':
          mode = INTERACTIVE;
          break;

        case 'p':
          mode = DAEMON;
          port = strtoul (optarg, NULL, 10);
          break;

	case 'O':
	  set_namespace (NS_OTHER, optarg);
	  break;

	case 'S':
	  set_namespace (NS_SHARED, optarg);
	  break;
	  
        case 't':
          timeout = strtoul (optarg, NULL, 10);
          break;

        case 'v':
          printf ("GNU imap4 daemon" "("PACKAGE " " VERSION ")\n");
          exit (0);
          break;

        default:
          break;
        }
    }

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
    /* list_append (bookie, mbox_record); */
    list_append (bookie, path_record);
  }

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

  if (mode == DAEMON)
    imap4d_daemon_init ();

  /* Make sure we are in the root.  */
  chdir ("/");

  /* Set up for syslog.  */
  openlog ("gnu-imap4d", LOG_PID, LOG_FACILITY);

  /* Redirect any stdout error from the library to syslog, they
     should not go to the client.  */
  mu_error_set_print (syslog_error_printer);

  umask (S_IROTH | S_IWOTH | S_IXOTH);  /* 007 */

  /* Actually run the daemon.  */
  if (mode == DAEMON)
    imap4d_daemon (maxchildren, port);
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
  FILE *ifile;

  /* Reset hup to exit.  */
  signal (SIGHUP, imap4d_signal);
  /* Timeout alarm.  */
  signal (SIGALRM, imap4d_signal);

  ifile = fdopen (infile, "r");
  ofile = fdopen (outfile, "w");
  if (!ofile || !ifile)
    imap4d_bye (ERR_NO_OFILE);

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
  pid_t pid;

  pid = fork ();
  if (pid == -1)
    {
      perror ("fork failed:");
      exit (1);
    }
  else if (pid > 0)
    exit (0);                   /* Parent exits.  */

  setsid ();                    /* Become session leader.  */

  signal (SIGHUP, SIG_IGN);     /* Ignore SIGHUP.  */

  /* The second fork is to guarantee that the daemon cannot acquire a
     controlling terminal.  */
  pid = fork ();
  if (pid == -1)
    {
      perror("fork failed:");
      exit (1);
    }
  else if (pid > 0)
    exit (0);                   /* Parent exits.  */

  /* Close inherited file descriptors.  */
  {
    size_t i;
#if defined(HAVE_SYSCONF) && defined(_SC_OPEN_MAX)
    size_t fdlimit = sysconf(_SC_OPEN_MAX);
#else
    size_t fdlimit = 64;
#endif
    for (i = 0; i < fdlimit; ++i)
      close (i);
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

/* Prints out usage information and exits the program */

static void
imap4d_usage (char *argv0)
{
  printf ("Usage: %s [OPTIONS]\n", argv0);
  printf ("Runs the GNU IMAP4 daemon.\n\n");
  printf ("  -d, --daemon=MAXCHILDREN runs in daemon mode with a maximum\n");
  printf ("                           of MAXCHILDREN child processes\n");
  printf ("  -h, --help               display this help and exit\n");
  printf ("  -i, --inetd              runs in inetd mode (default)\n");
  printf ("  -p, --port=PORT          specifies port to listen on, implies -d\n"
);
  printf ("                           defaults to 143, which need not be specifi
ed\n");
  printf ("  -t, --timeout=TIMEOUT    sets idle timeout to TIMEOUT seconds\n");
  printf ("                           TIMEOUT default is 1800 (30 minutes)\n");
  printf ("  -v, --version            display version information and exit\n");
  printf ("\nReport bugs to bug-mailutils@gnu.org\n");
  exit (0);
}

static int
syslog_error_printer (const char *fmt, va_list ap)
{
  vsyslog (LOG_CRIT, fmt, ap);
  return 0;
}

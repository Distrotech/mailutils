/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include "pop3d.h"

#ifdef HAVE_MYSQL
# include "../MySql/MySql.h"
#endif

mailbox_t mbox;
int state;
char *username;
FILE *ifile;
FILE *ofile;
char *md5shared;

struct daemon_param daemon_param = {
  MODE_INTERACTIVE,     /* Start in interactive (inetd) mode */
  20,                   /* Default maximum number of children */
  110,                  /* Standard POP3 port */
  600                   /* Idle timeout */
};

/* Number of child processes.  */
volatile size_t children;

static int pop3d_mainloop       __P ((int, int));
static void pop3d_daemon_init   __P ((void));
static void pop3d_daemon        __P ((unsigned int, unsigned int));

const char *argp_program_version = "pop3d (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU pop3d -- the POP3 daemon";

static struct argp argp = {
  NULL,
  NULL,
  NULL, 
  doc,
  mu_daemon_argp_child,
  NULL, NULL
};


int
main (int argc, char **argv)
{
  struct group *gr;
  int status = OK;
  
  mu_create_argcv (argc, argv, &argc, &argv);
  argp_parse (&argp, argc, argv, 0, 0, &daemon_param);

  maildir = mu_normalize_maildir (maildir);
  if (!maildir)
    {
      mu_error ("Badly formed maildir: %s", maildir);
      exit (1);
    }
	    
  /* First we want our group to be mail so we can access the spool.  */
  gr = getgrnam ("mail");
  if (gr == NULL)
    {
      perror ("Error getting mail group");
      exit (EXIT_FAILURE);
    }

  if (setgid (gr->gr_gid) == -1)
    {
      perror ("Error setting mail group");
      exit (EXIT_FAILURE);
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
#endif
#ifdef USE_VIRTUAL_DOMAINS
  mu_register_getpwnam (getpwnam_virtual);
  mu_register_getpwnam (getpwnam_ip_virtual);
  mu_register_getpwnam (getpwnam_host_virtual);
#endif

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

  /* Actually run the daemon.  */
  if (daemon_param.mode == MODE_DAEMON)
    pop3d_daemon (daemon_param.maxchildren, daemon_param.port);
  /* exit (EXIT_SUCCESS) -- no way out of daemon except a signal.  */
  else
    status = pop3d_mainloop (fileno (stdin), fileno (stdout));

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
      perror ("failed to become a daemon:");
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

/* The main part of the daemon. This function reads input from the client and
   executes the proper functions. Also handles the bulk of error reporting.  */
static int
pop3d_mainloop (int infile, int outfile)
{
  int status = OK;

  /* Reset hup to exit.  */
  signal (SIGHUP, pop3d_signal);
  /* Timeout alarm.  */
  signal (SIGALRM, pop3d_signal);

  ifile = fdopen (infile, "r");
  ofile = fdopen (outfile, "w");
  if (!ifile || !ofile)
    pop3d_abquit (ERR_NO_OFILE);

  state = AUTHORIZATION;

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
  fprintf (ofile, "+OK POP3 Ready %s\r\n", md5shared);

  while (state != UPDATE)
    {
      char *buf, *arg, *cmd;

      fflush (ofile);
      status = OK;
      buf = pop3d_readline (ifile);
      cmd = pop3d_cmd (buf);
      arg = pop3d_args (buf);

      /* The mailbox size needs to be check to make sure that we are in
	 sync.  Some other applications may not respect the *.lock or
	 the lock may be stale because downloading on slow modem.
	 We rely on the size of the mailbox for the check and bail if out
	 of sync.  */
      if (state == TRANSACTION && !mailbox_is_updated (mbox))
	{
	  static off_t mailbox_size;
	  off_t newsize = 0;
	  mailbox_get_size (mbox, &newsize);
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
      else
	status = ERR_BAD_CMD;

      if (status == OK)
	; /* Everything is good.  */
      else if (status == ERR_WRONG_STATE)
	fprintf (ofile, "-ERR " BAD_STATE "\r\n");
      else if (status == ERR_BAD_ARGS)
	fprintf (ofile, "-ERR " BAD_ARGS "\r\n");
      else if (status == ERR_NO_MESG)
	fprintf (ofile, "-ERR " NO_MESG "\r\n");
      else if (status == ERR_MESG_DELE)
	fprintf (ofile, "-ERR " MESG_DELE "\r\n");
      else if (status == ERR_NOT_IMPL)
	fprintf (ofile, "-ERR " NOT_IMPL "\r\n");
      else if (status == ERR_BAD_CMD)
	fprintf (ofile, "-ERR " BAD_COMMAND "\r\n");
      else if (status == ERR_BAD_LOGIN)
	fprintf (ofile, "-ERR " BAD_LOGIN "\r\n");
      else if (status == ERR_MBOX_LOCK)
	fprintf (ofile, "-ERR [IN-USE] " MBOX_LOCK "\r\n");
      else if (status == ERR_TOO_LONG)
	fprintf (ofile, "-ERR " TOO_LONG "\r\n");
      else if (status == ERR_FILE)
	fprintf (ofile, "-ERR " FILE_EXP "\r\n");
      else
	fprintf (ofile, "-ERR unknown error\r\n");

      free (cmd);
      free (arg);
    }

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

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
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
          status = pop3d_mainloop (connfd, connfd);
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





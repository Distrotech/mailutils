/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

/* save some line space */
typedef struct sockaddr_in SA;

/* number of child processes */
unsigned int children = 0;

static struct option long_options[] =
{
  {"daemon", 2, 0, 'd'},
  {"help", 0, 0, 'h'},
  {"inetd", 0, 0, 'i'},
  {"port", 1, 0, 'p'},
  {"timeout", 1, 0, 't'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};

int
main (int argc, char **argv)
{
  struct group *gr;
  static int mode = INTERACTIVE;
  int maxchildren = 10;
  int option_index = 0;
  int c = 0;

  port = 110;			/* Default POP3 port */
  timeout = 0;			/* Default timeout of 0 */

  while ((c = getopt_long (argc, argv, "d::hip:t:v", long_options, &option_index)) && c != -1)
    {
      switch (c)
	{
	case 'd':
	  mode = DAEMON;
	  maxchildren = optarg ? atoi (optarg) : 10;
	  if (maxchildren <= 0)
          maxchildren = 10;
	  break;
	case 'h':
	  pop3_usage (argv[0]);
	  break;
	case 'i':
	  mode = INTERACTIVE;
	  break;
	case 'p':
	  mode = DAEMON;
	  port = atoi (optarg);
	  break;
	case 't':
	  timeout = atoi (optarg);
	  break;
	case 'v':
	  printf (IMPL " ("PACKAGE " " VERSION ")\n");
	  exit (0);
	  break;
	default:
	  break;
	}
    }

  /* First we want our group to be mail so we can access the spool */
  gr = getgrnam ("mail");
  if (gr == NULL)
    {
      perror ("Error getting group");
      exit (1);
    }

  if (setgid (gr->gr_gid) == -1)
    {
      perror ("Error setting group");
      exit (1);
    }

  /* Set the signal handlers */
  signal (SIGINT, pop3_signal);
  signal (SIGQUIT, pop3_signal);
  signal (SIGILL, pop3_signal);
  signal (SIGBUS, pop3_signal);
  signal (SIGFPE, pop3_signal);
  signal (SIGSEGV, pop3_signal);
  signal (SIGTERM, pop3_signal);
  signal (SIGSTOP, pop3_signal);

  if (timeout < 600)		/* RFC 1939 says no less than 10 minutes */
    timeout = 0;		/* So we'll turn it off */

  if (mode == DAEMON)
    pop3_daemon_init ();

  /* change directories */
#ifdef MAILSPOOLHOME
  chdir ("/");
#else
  chdir (_PATH_MAILDIR);
#endif

  /* Set up for syslog */
  openlog ("gnu-pop3d", LOG_PID, LOG_MAIL);

  umask (S_IROTH | S_IWOTH | S_IXOTH);	/* 007 */

  /* Actually run the daemon */
  if (mode == DAEMON)
      pop3_daemon (maxchildren);
      /* exit() -- no way out of daemon except a signal */
  else
      pop3_mainloop (fileno (stdin), fileno (stdout));

  /* Close the syslog connection and exit */
  closelog ();
  return OK;
}

/* Sets things up for daemon mode */

void
pop3_daemon_init (void)
{
  pid_t pid;
  unsigned int i;
#define MAXFD 64

  pid = fork();
  if (pid == -1)
    {
      perror("fork failed:");
      exit (-1);
    }
  else if (pid > 0)
      exit (0);			/* parent exits */
  
  setsid ();			/* become session leader */

  signal (SIGHUP, SIG_IGN);	/* ignore SIGHUP */

  pid = fork();
  if (pid == -1)
    {
      perror("fork failed:");
      exit (-1);
    }
  else if (pid > 0)
      exit (0);			/* parent exits */
  
  /* close inherited file descriptors */
  for (i = 0; i < MAXFD; ++i)
      close(i);

  signal (SIGCHLD, pop3_sigchld);
}

/* The main part of the daemon. This function reads input from the client and
   executes the proper functions. Also handles the bulk of error reporting. */

int
pop3_mainloop (int infile, int outfile)
{
  int status = OK;
  char *buf, *arg, *cmd;
  struct hostent *htbuf;
  char *local_hostname;

  ifile = infile;
  ofile = fdopen (outfile, "w");
  if (ofile == NULL)
    pop3_abquit (ERR_NO_OFILE);
  state = AUTHORIZATION;
  curr_time = time (NULL);

  syslog (LOG_INFO, "Incoming connection opened");

  /* Prepare the shared secret for APOP */
  local_hostname = malloc (MAXHOSTNAMELEN + 1);
  if (local_hostname == NULL)
    pop3_abquit (ERR_NO_MEM);

  gethostname (local_hostname, MAXHOSTNAMELEN);
  htbuf = gethostbyname (local_hostname);
  if (htbuf)
    {
      free (local_hostname);
      local_hostname = strdup (htbuf->h_name);
    }

  md5shared = malloc (strlen (local_hostname) + 51);
  if (md5shared == NULL)
    pop3_abquit (ERR_NO_MEM);

  snprintf (md5shared, strlen (local_hostname) + 50, "<%d.%ld@%s>", getpid (),
	    time (NULL), local_hostname);
  free (local_hostname);

  fflush (ofile);
  fprintf (ofile, "+OK POP3 " WELCOME " %s\r\n", md5shared);

  while (state != UPDATE)
    {
      fflush (ofile);
      status = OK;
      buf = pop3_readline (ifile);
      cmd = pop3_cmd (buf);
      arg = pop3_args (buf);

      if (strlen (arg) > POP_MAXCMDLEN || strlen (cmd) > POP_MAXCMDLEN)
	status = ERR_TOO_LONG;
      else if (strlen (cmd) > 4)
	status = ERR_BAD_CMD;
      else if (strncasecmp (cmd, "RETR", 4) == 0)
	status = pop3_retr (arg);
      else if (strncasecmp (cmd, "DELE", 4) == 0)
	status = pop3_dele (arg);
      else if (strncasecmp (cmd, "USER", 4) == 0)
	status = pop3_user (arg);
      else if (strncasecmp (cmd, "QUIT", 4) == 0)
	status = pop3_quit (arg);
      else if (strncasecmp (cmd, "APOP", 4) == 0)
	status = pop3_apop (arg);
      else if (strncasecmp (cmd, "AUTH", 4) == 0)
	status = pop3_auth (arg);
      else if (strncasecmp (cmd, "STAT", 4) == 0)
	status = pop3_stat (arg);
      else if (strncasecmp (cmd, "LIST", 4) == 0)
	status = pop3_list (arg);
      else if (strncasecmp (cmd, "NOOP", 4) == 0)
	status = pop3_noop (arg);
      else if (strncasecmp (cmd, "RSET", 4) == 0)
	status = pop3_rset (arg);
      else if ((strncasecmp (cmd, "TOP", 3) == 0) && (strlen (cmd) == 3))
	status = pop3_top (arg);
      else if (strncasecmp (cmd, "UIDL", 4) == 0)
	status = pop3_uidl (arg);
      else if (strncasecmp (cmd, "CAPA", 4) == 0)
	status = pop3_capa (arg);
      else
	status = ERR_BAD_CMD;

      if (status == OK)
	fflush (ofile);
      else if (status == ERR_WRONG_STATE)
	fprintf (ofile, "-ERR " BAD_STATE "\r\n");
      else if (status == ERR_BAD_ARGS)
	fprintf (ofile, "-ERR " BAD_ARGS "\r\n");
      else if (status == ERR_NO_MESG)
	fprintf (ofile, "-ERR " NO_MESG "\r\n");
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
	  else
	fprintf (ofile, "-ERR unknown error\r\n");

      free (buf);
      free (cmd);
      free (arg);
    }

  fflush (ofile);
  return OK;
}

/* Runs GNU POP3 in standalone daemon mode. This opens and binds to a port 
   (default 110) then executes a pop3_mainloop() upon accepting a connection.
   It starts maxchildren child processes to listen to and accept socket
   connections */

void
pop3_daemon (unsigned int maxchildren)
{
  SA server, client;
  pid_t pid;
  int listenfd, connfd;
  size_t size;

  if ( (listenfd = socket (AF_INET, SOCK_STREAM, 0)) == -1 )
    {
      syslog (LOG_ERR, "socket: %s", strerror(errno));
	  exit (-1);
    }
  size = 1; /* use size here to avoid making a new variable */
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &size, sizeof(size));  
  size = sizeof(server);
  memset (&server, 0, size);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htonl (port);

  if (bind(listenfd, (SA *) &server, size) == -1 )
    {
      syslog(LOG_ERR, "bind: %s", strerror(errno));
      exit(-1);
    }

  if (listen(listenfd, 128) == -1)
    {
      syslog(LOG_ERR, "listen: %s", strerror(errno));
	  exit(-1);
    }
  
  for ( ; ; )
    {
      if (children > maxchildren)
        {
          pause();
          continue;
        }
      if ( (connfd = accept(listenfd, (SA *) &client, &size)) == -1)
        {
          if (errno == EINTR)
              continue;
          syslog(LOG_ERR, "accept: %s", strerror(errno));
          exit(-1);
        }

      pid = fork();
      if (pid == -1)
          syslog(LOG_ERR, "fork: %s", strerror(errno));
      else if(pid == 0) /* child */
        {
          close(listenfd);
		  /* syslog(); FIXME log the info on the connectiing client */
          pop3_mainloop(connfd, connfd);
        }
      else
        {
          ++children;
        }

	  close(connfd);
    }
}

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
  static int mode;
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
  switch (mode)
    {
    case DAEMON:
      if (maxchildren < 10)
	maxchildren = 10;
      pop3_daemon (maxchildren);
      break;
    case INTERACTIVE:
    default:
      pop3_mainloop (fileno (stdin), fileno (stdout));
      break;
    }

  /* Close the syslog connection and exit */
  closelog ();
  return OK;
}

/* Sets things up for daemon mode */

void
pop3_daemon_init (void)
{
  if (fork ())
    exit (0);			/* parent exits */
  setsid ();			/* become session leader */

  if (fork ())
    exit (0);			/* new parent exits */

  /* close inherited file descriptors */
  close (0);
  close (1);
  close (2);

  signal (SIGHUP, SIG_IGN);	/* ignore SIGHUP */
  signal (SIGCHLD, pop3_signal);	/* for forking */
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
  ofile = fdopen (outfile, "w+");
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
      else if (strncasecmp (cmd, "RETR", 4) == 0)
	status = pop3_retr (arg);
      else if (strncasecmp (cmd, "DELE", 4) == 0)
	status = pop3_dele (arg);
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

      free (buf);
      free (cmd);
      free (arg);
    }

  fflush (ofile);
  return OK;
}

/* Runs GNU POP in standalone daemon mode. This opens and binds to a port 
   (default 110) then executes a pop3_mainloop() upon accepting a connection.
   It starts maxchildren child processes to listen to and accept socket
   connections */

int
pop3_daemon (int maxchildren)
{
  int children = 0;
  struct sockaddr_in client;
  int sock, sock2;
  unsigned int socksize;

  sock = socket (PF_INET, SOCK_STREAM, (getprotobyname ("tcp"))->p_proto);
  if (sock < 0)
    {
      syslog (LOG_ERR, "%s\n", strerror (errno));
      exit (1);
    }
  memset (&client, 0, sizeof (struct sockaddr_in));
  client.sin_family = AF_INET;
  client.sin_port = htons (port);
  client.sin_addr.s_addr = htonl (INADDR_ANY);
  socksize = sizeof (client);
  if (bind (sock, (struct sockaddr *) &client, socksize))
    {
      perror ("Couldn't bind to socket");
      exit (1);
    }
  listen (sock, 128);
  while (1)
    {
      if (children < maxchildren)
	{
	  if (!fork ())
	    {
	      sock2 = accept (sock, &client, &socksize);
	      pop3_mainloop (sock2, sock2);
	      close (sock2);
	      exit (OK);
	    }
	  else
	    {
	      /* wait (NULL); */
	      children++;
	    }
	}
      else
	{
	  wait (NULL);
	  children--;
	}
    }
  return OK;
}

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

#include "comsat.h"

#define	IMPL		"GNU Comsat Daemon"

#ifndef PATH_DEV
# define PATH_DEV "/dev"
#endif
#ifndef PATH_TTY_PFX
# define PATH_TTY_PFX PATH_DEV
#endif

#ifdef HAVE_UTMP_H
# include <utmp.h>
#endif

#ifdef UTMPX
# ifdef HAVE_UTMPX_H
#  include <utmpx.h>
# endif
typedef struct utmpx UTMP;
# define SETUTENT() setutxent()
# define GETUTENT() getutxent()
# define ENDUTENT() endutxent()
#else
typedef struct utmp UTMP;
# define SETUTENT() setutent()
# define GETUTENT() getutent()
# define ENDUTENT() endutent()
#endif

#define MAX_TTY_SIZE (sizeof (PATH_TTY_PFX) + sizeof (((UTMP*)0)->ut_line))

static char short_options[] = "c:dhip:t:v";
static struct option long_options[] =
{
  {"config", required_argument, 0, 'c'},
  {"daemon", no_argument, 0, 'd'},
  {"help", no_argument, 0, 'h'},
  {"inetd", no_argument, 0, 'i'},
  {"port", required_argument, 0, 'p'},
  {"timeout", required_argument, 0, 't'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

#define MODE_INETD 0
#define MODE_DAEMON 1

#define SUCCESS 0
#define NOT_HERE 1
#define PERMISSION_DENIED 2

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

int mode = MODE_INETD;
int port = 512; /* Default biff port */
int timeout = 0;
int maxlines = 5;
char hostname[MAXHOSTNAMELEN];
char *username;

static void comsat_init (void);
static void comsat_daemon_init (void);
static void comsat_daemon (int port);
static int comsat_main (int fd);
static void notify_user (char *user, char *device, char *path, off_t offset);
static int find_user (char *name, char *tty);
static void help (void);
char *mailbox_path (const char *user);
void change_user (char *user);

static int xargc;
static char **xargv;

int
main(int argc, char **argv)
{
  int c;
  char *config_file = NULL;

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 'c':
	  config_file = optarg;
	  break;
	case 'd':
	  mode = MODE_DAEMON;
	  break;
	case 'h':
	  help ();
	  /*NOTREACHED*/
	case 'i':
	  mode = MODE_INETD;
	  break;
	case 'p':
	  port = strtoul (optarg, NULL, 10);
	  break;
	case 't':
	  timeout = strtoul (optarg, NULL, 10);
	  break;
	case 'v':
	  printf (IMPL " ("PACKAGE " " VERSION ")\n");
	  exit (EXIT_SUCCESS);
	  break;
	default:
	  exit (EXIT_FAILURE);
	}
    }

  if (timeout > 0 && mode == MODE_DAEMON)
    {
      fprintf (stderr, "--timeout and --daemon are incompatible\n");
      exit (EXIT_FAILURE);
    }

  comsat_init ();

  if (mode == MODE_DAEMON)
    {
      /* Preserve invocation arguments */
      xargc = argc;
      xargv = argv;
      comsat_daemon_init ();
    }

  /* Set up error messaging  */
  openlog ("gnu-comsat", LOG_PID, LOG_LOCAL1);
  mu_error_set_print (mu_syslog_error_printer);

  if (config_file)
    read_config (config_file);

  chdir ("/");

  if (mode == MODE_DAEMON)
    comsat_daemon (port);
  else
    c = comsat_main (0);

  return c != 0;
}

RETSIGTYPE
sig_hup (int sig)
{
  syslog (LOG_NOTICE, "restarting");

  if (xargv[0][0] != '/')
    syslog (LOG_ERR, "can't restart: not started with absolute pathname");
  else
    execvp (xargv[0], xargv);

  signal (sig, sig_hup);
}

void
comsat_init ()
{
  list_t bookie;

  registrar_get_list (&bookie);
  /* list_append (bookie, mbox_record); */
  list_append (bookie, path_record);

  gethostname (hostname, sizeof hostname);

  /* Set signal handlers */
  signal (SIGTTOU, SIG_IGN);
  signal (SIGCHLD, SIG_IGN);
  signal (SIGHUP, SIG_IGN);	/* Ignore SIGHUP.  */
}

/* Set up for daemon mode.  */
static void
comsat_daemon_init (void)
{
  extern int daemon (int, int);

  /* Become a daemon. Take care to close inherited fds and to hold
     first three one, in, out, err.  Do not do the chdir("/").   */
  if (daemon (1, 0) < 0)
    {
      perror ("failed to become a daemon:");
      exit (EXIT_FAILURE);
    }
}

int allow_biffrc = 1;            /* Allow per-user biffrc files */
unsigned maxrequests = 16;       /* Maximum number of request allowed per
			            control interval */
time_t request_control_interval = 10;  /* Request control interval */
time_t overflow_control_interval = 10; /* Overflow control interval */
time_t overflow_delay_time = 5;


void
comsat_daemon (int port)
{
  int fd;
  struct sockaddr_in local_sin;
  time_t last_request_time;    /* Timestamp of the last received request */
  unsigned reqcount = 0;       /* Number of request received in the
				  current control interval */
  time_t last_overflow_time;   /* Timestamp of last overflow */
  unsigned overflow_count = 0; /* Number of overflows achieved during
				  the current interval */
  time_t now;

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    {
      syslog (LOG_CRIT, "socket: %m");
      exit (1);
    }

  memset (&local_sin, 0, sizeof local_sin);
  local_sin.sin_family = AF_INET;
  local_sin.sin_addr.s_addr = INADDR_ANY; /*FIXME*/
  local_sin.sin_port = htons (port);

  if (bind (fd, (struct sockaddr *) &local_sin, sizeof local_sin) < 0)
    {
      syslog (LOG_CRIT, "bind: %m");
      exit (1);
    }

  syslog (LOG_NOTICE, "GNU comsat started");

  last_request_time = last_overflow_time = time (NULL);
  while (1)
    {
      fd_set fdset;
      int rc;

      FD_ZERO (&fdset);
      FD_SET (fd, &fdset);
      rc = select (fd+1, &fdset, NULL, NULL, NULL);
      if (rc == -1)
	{
	  if (errno != EINTR)
	    syslog (LOG_ERR, "select: %m");
	  continue;
	}

      /* Control the request flow */
      if (maxrequests != 0)
	{
	  now = time (NULL);
	  if (reqcount > maxrequests)
	    {
	      unsigned delay;

	      delay = overflow_delay_time << (overflow_count + 1);
	      syslog (LOG_NOTICE, "too many requests: pausing for %u seconds",
		      delay);
	      sleep (delay);
	      reqcount = 0;
	      if (now - last_overflow_time <= overflow_control_interval)
		{
		  if ((overflow_delay_time << (overflow_count + 2)) >
		      overflow_delay_time)
		    ++overflow_count;
		}
	      else
		overflow_count = 0;
	      last_overflow_time = time (NULL);
	    }

	  if (now - last_request_time <= request_control_interval)
	    reqcount++;
	  else
	    {
	      last_request_time = now;
	      reqcount = 1;
	    }
	}
      comsat_main (fd);
    }
}

int
comsat_main (int fd)
{
  int rdlen;
  int len;
  struct sockaddr_in sin_from;
  char buffer[216]; /*FIXME: Arbitrary size */
  pid_t pid;
  char tty[MAX_TTY_SIZE];
  char *p, *endp;
  size_t offset;
  char *path = NULL;

  len = sizeof sin_from;
  rdlen = recvfrom (fd, buffer, sizeof buffer, 0,
		    (struct sockaddr*)&sin_from, &len);
  if (rdlen <= 0)
    {
      if (errno == EINTR)
	return 0;
      syslog (LOG_ERR, "recvfrom: %m");
      return 1;
    }

  if (acl_match (&sin_from))
    {
      syslog (LOG_ALERT, "DENIED attempt to connect from %s",
	      inet_ntoa (sin_from.sin_addr));
      return 1;
    }

  syslog (LOG_INFO, "%d bytes from %s", rdlen, inet_ntoa (sin_from.sin_addr));

  buffer[rdlen] = 0;

  /* Parse the buffer */
  p = strchr (buffer, '@');
  if (!p)
    {
      syslog (LOG_ERR, "malformed input: %s", buffer);
      return 1;
    }
  *p++ = 0;

  offset = strtoul (p, &endp, 0);
  switch (*endp)
    {
    case 0:
      break;
    case ':':
      path = endp+1;
      break;
    default:
      if (!isspace (*endp))
	syslog (LOG_ERR, "malformed input: %s@%s (near %s)", buffer, p, endp);
    }

  if (find_user (buffer, tty) != SUCCESS)
    return 0;

  /* All I/O is done by child process. This is to avoid various blocking
     problems. */

  pid = fork ();

  if (pid == -1)
    {
      syslog (LOG_ERR, "fork: %m");
      return 1;
    }

  if (pid > 0)
    {
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      select (0, NULL, NULL, NULL, &tv);
      kill (pid, SIGKILL); /* Just in case the child is hung */
      return 0;
    }

  /* Child: do actual I/O */
  notify_user (buffer, tty, path, offset);
  exit (0);
}

char *
get_newline_str (FILE *fp)
{
#if defined(OPOST) && defined(ONLCR)
  struct termios tbuf;

  tcgetattr (fileno (fp), &tbuf);
  if ((tbuf.c_oflag & OPOST) && (tbuf.c_oflag & ONLCR))
    return "\n";
  else
    return "\r\n";
#else
  return "\r\n"; /* Just in case */
#endif
}

/* NOTE: Do not bother to free allocated memory, as the program exits
   immediately after executing this */
void
notify_user (char *user, char *device, char *path, off_t offset)
{
  FILE *fp;
  char *cr, *p, *blurb;
  mailbox_t mbox = NULL, tmp = NULL;
  message_t msg;
  body_t body = NULL;
  header_t header = NULL;
  stream_t stream = NULL;
  int status;
  size_t size, count, n;

  change_user (user);
  if ((fp = fopen (device, "w")) == NULL)
    {
      syslog (LOG_ERR, "can't open device %s: %m", device);
      exit (0);
    }

  cr = get_newline_str (fp);

  if (!path)
    {
      path = mailbox_path (user);
      if (!path)
	return;
    }

  if ((status = mailbox_create (&mbox, path)) != 0
      || (status = mailbox_open (mbox, MU_STREAM_READ)) != 0)
    {
      syslog (LOG_ERR, "can't open mailbox %s: %s",
	      path, strerror (status));
      return;
    }

  mailbox_destroy_folder (mbox);
  if ((status = mailbox_get_stream (mbox, &stream)))
    {
      syslog (LOG_ERR, "can't get stream for mailbox %s: %s",
	      path, strerror (status));
      return;
    }

  if ((status = stream_size (stream, (off_t *) &size)))
    {
      syslog (LOG_ERR, "can't get stream size (mailbox %s): %s",
	      path, strerror (status));
      return;
    }

  /* Read headers */
  size -= offset;
  blurb = malloc (size + 1);
  if (!blurb)
    return;

  stream_read (stream, blurb, size, offset, &n);
  blurb[size] = 0;

  if (mailbox_create (&tmp, "/dev/null") != 0
      || mailbox_open (tmp, MU_STREAM_READ) != 0)
    {
      syslog (LOG_ERR, "can't create temporary mailbox: %s",
	      strerror (status));
      return;
    }

  if ((status = memory_stream_create (&stream)))
    {
      syslog (LOG_ERR, "can't create temporary stream: %s",
	      strerror (status));
      return;
    }

  stream_write (stream, blurb, size, 0, &count);
  mailbox_destroy_folder (tmp);
  mailbox_set_stream (tmp, stream);
  mailbox_messages_count (tmp, &count);
  mailbox_get_message (tmp, 1, &msg);

  run_user_action (fp, cr, msg);
  fclose (fp);
}

/* Search utmp for the local user */
int
find_user (char *name, char *tty)
{
  UTMP *uptr;
  int status;
  struct stat statb;
  char ftty[MAX_TTY_SIZE];
  time_t last_time = 0;

  status = NOT_HERE;
  sprintf (ftty, "%s/", PATH_TTY_PFX);

  SETUTENT ();

  while ((uptr = GETUTENT ()) != NULL)
    {
#ifdef USER_PROCESS
      if (uptr->ut_type != USER_PROCESS)
	continue;
#endif
      if (!strncmp (uptr->ut_name, name, sizeof(uptr->ut_name)))
	{
	  /* no particular tty was requested */
	  strncpy (ftty + sizeof(PATH_DEV),
		   uptr->ut_line,
		   sizeof (ftty) - sizeof (PATH_DEV) - 2);
	  ftty[sizeof (ftty) - 1] = 0;

	  mu_normalize_path (ftty, "/");
	  if (strncmp (ftty, PATH_TTY_PFX, strlen(PATH_TTY_PFX)))
	    {
	      /* An attempt to break security... */
	      syslog (LOG_ALERT, "bad line name in utmp record: %s", ftty);
	      return NOT_HERE;
	    }

	  if (stat (ftty, &statb) == 0)
	    {
	      if (!S_ISCHR (statb.st_mode))
		{
		  syslog (LOG_ALERT, "not a character device: %s", ftty);
		  return NOT_HERE;
		}

	      if (!(statb.st_mode & S_IEXEC))
		{
		  if (status != SUCCESS)
		    status = PERMISSION_DENIED;
		  continue;
		}
	      if (statb.st_atime > last_time)
		{
		  last_time = statb.st_atime;
		  strcpy(tty, ftty);
		  status = SUCCESS;
		}
	      continue;
	    }
	}
    }

  ENDUTENT ();
  return status;
}

void
change_user (char *user)
{
  struct passwd *pw;

  pw = getpwnam (user);
  if (!pw)
    {
      syslog (LOG_CRIT, "no such user: %s", user);
      exit (1);
    }

  setgid (pw->pw_gid);
  setuid (pw->pw_uid);
  chdir (pw->pw_dir);
  username = user;
}

void
help ()
{
  printf ("Usage: comsatd [OPTIONS]\n");
  printf ("Options are:\n");
  printf ("  -c, --config=PATH      read configuration from the file\n");
  printf ("  -d, --daemon           run in daemon mode\n");
  printf ("  -h, --help             display this help and exit\n");
  printf ("  -i, --inetd            run in inetd mode (default)\n");
  printf ("  -p, --port=PORT        specify port to listen on, implies -d\n");
  printf ("  -t, --timeout=VALUE    set idle timeout (implies -i)\n");
  printf ("  -v, --version          display version information and exit\n");
  printf ("\nReport bugs to bug-mailutils@gnu.org\n");
  exit (EXIT_SUCCESS);
}

char *
mailbox_path (const char *user)
{
  struct passwd *pw;
  char *mailbox_name;

  pw = mu_getpwnam (user);
  if (!pw)
    {
      syslog (LOG_ALERT, "user nonexistent: %s", user);
      return NULL;
    }

  if (!mu_virtual_domain)
    {
      mailbox_name = calloc (strlen (_PATH_MAILDIR) + 1
			     + strlen (pw->pw_name) + 1, 1);
      sprintf (mailbox_name, "%s/%s", _PATH_MAILDIR, pw->pw_name);
    }
  else
    {
      mailbox_name = calloc (strlen (pw->pw_dir) + strlen ("/INBOX"), 1);
      sprintf (mailbox_name, "%s/INBOX", pw->pw_dir);
    }
  return mailbox_name;
}

#if 0
/* A debugging hook */
volatile int _st=0;
void
stop()
{
  syslog (LOG_ALERT, "waiting for debug");
  while (!_st)
    _st=_st;
}
#endif

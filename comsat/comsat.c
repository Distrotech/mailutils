/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 
   2007 Free Software Foundation, Inc.

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

#include "comsat.h"
#include "mailutils/libargp.h"

#ifndef PATH_DEV
# define PATH_DEV "/dev"
#endif
#ifndef PATH_TTY_PFX
# define PATH_TTY_PFX PATH_DEV
#endif

#ifdef HAVE_UTMP_H
# include <utmp.h>
#endif

#ifndef HAVE_GETUTENT_CALLS
extern void setutent (void);
extern struct utmp *getutent (void);
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

const char *program_version = "comsatd (" PACKAGE_STRING ")";
static char doc[] = "GNU comsatd";

static struct argp_option options[] = 
{
  { "config", 'c', N_("FILE"), OPTION_HIDDEN, "", 0 },
  { "convert-config", 'C', N_("FILE"), 0,
    N_("Convert the configuration FILE to new format."), 0 },
  { "test", 't', NULL, 0, N_("Run in test mode"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t comsatd_parse_opt (int key, char *arg,
				  struct argp_state *state);

static struct argp argp = {
  options,
  comsatd_parse_opt,
  NULL, 
  doc,
  NULL,
  NULL, NULL
};

static const char *comsat_argp_capa[] = {
  "daemon",
  "common",
  "debug",
  "logging",
  "mailbox",
  "locking",
  "license",
  NULL
};

#define SUCCESS 0
#define NOT_HERE 1
#define PERMISSION_DENIED 2

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

struct mu_gocs_daemon default_gocs_daemon = {
  MODE_INTERACTIVE,     /* Start in interactive (inetd) mode */
  20,                   /* Default maximum number of children.
			   Currently unused */
  512,                  /* Default biff port */
  0,                    /* Default timeout */
};
int maxlines = 5;
char hostname[MAXHOSTNAMELEN];
const char *username;
mu_acl_t comsat_acl;

static void comsat_init (void);
static void comsat_daemon_init (void);
static void comsat_daemon (int _port);
static int comsat_main (int fd);
static void notify_user (const char *user, const char *device,
			 const char *path, mu_message_qid_t qid);
static int find_user (const char *name, char *tty);
static char *mailbox_path (const char *user);
static void change_user (const char *user);

static int xargc;
static char **xargv;
int test_mode;

struct mu_cfg_param comsat_cfg_param[] = {
  { "allow-biffrc", mu_cfg_bool, &allow_biffrc, 0, NULL,
    N_("Read .biffrc file from the user home directory") },
  { "max-lines", mu_cfg_int, &maxlines, 0, NULL,
    N_("Maximum number of message body lines to be output.") },
  { "max-requests", mu_cfg_uint, &maxrequests, 0, NULL,
    N_("Maximum number of incoming requests per `request-control-interval' "
       "seconds.") },
  { "request-control-interval", mu_cfg_time, &request_control_interval,
    0, NULL,
    N_("Set control interval.") },
  { "overflow-control-interval", mu_cfg_time, &overflow_control_interval,
    0, NULL,
    N_("Set overflow control interval.") },
  { "overflow-delay-time", mu_cfg_time, &overflow_delay_time,
    0, NULL,
    N_("Time to sleep after the first overflow occurs.") },
  { "acl", mu_cfg_section, },
  { NULL }
};

static error_t
comsatd_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'c':
      {
	char *cfg;
	int fd;
	FILE *fp;

	mu_diag_output (MU_DIAG_WARNING,
_("The old configuration file format and the --config command\n"
  "line option are deprecated and will be removed in the future\n"
  "release. Please use --convert-config option to convert your\n"
  "settings to the new format."));
	/* FIXME: Refer to the docs */
	
	fd = mu_tempfile (NULL, &cfg);
	fp = fdopen (fd, "w");
	convert_config (arg, fp);
	fclose (fp);
	mu_get_config (cfg, mu_program_name, comsat_cfg_param, 0, NULL);
	unlink (cfg);
	free (cfg);
      }
      break;
      
    case 'C':
      convert_config (arg, stdout);
      exit (0);

    case 't':
      test_mode = 1;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int c;
  int ind;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
  mu_gocs_daemon = default_gocs_daemon;
  comsat_init ();
  mu_acl_cfg_init ();

  if (mu_app_init (&argp, comsat_argp_capa, comsat_cfg_param, argc, argv, 0,
		   &ind, &comsat_acl))
    exit (1);

  argc -= ind;
  argv += ind;
  
  if (test_mode)
    {
      char *user;
      
      if (argc < 2 || argc > 2)
	{
	  mu_error (_("Mailbox URL and message QID are required in test mode"));
	  exit (EXIT_FAILURE);
	}

      user = getenv ("LOGNAME");
      if (!user)
	{
	  user = getenv ("USER");
	  if (!user)
	    {
	      struct passwd *pw = getpwuid (getuid ());
	      if (!pw)
		{
		  mu_error (_("Cannot determine user name"));
		  exit (EXIT_FAILURE);
		}
	      user = pw->pw_name;
	    }
	}
		  
      notify_user (user, "/dev/tty", argv[0], argv[1]);
      exit (0);
    }
  
  if (mu_gocs_daemon.timeout > 0 && mu_gocs_daemon.mode == MODE_DAEMON)
    {
      mu_error (_("--timeout and --daemon are incompatible"));
      exit (EXIT_FAILURE);
    }

  if (mu_gocs_daemon.mode == MODE_DAEMON)
    {
      /* Preserve invocation arguments */
      xargc = argc;
      xargv = argv;
      comsat_daemon_init ();
    }

  /* Set up error messaging  */
  openlog ("gnu-comsat", LOG_PID, log_facility);

  {
    mu_debug_t debug;

    mu_diag_get_debug (&debug);
    mu_debug_set_print (debug, mu_diag_syslog_printer, NULL);
  }

  chdir ("/");

  if (mu_gocs_daemon.mode == MODE_DAEMON)
    comsat_daemon (mu_gocs_daemon.port);
  else
    c = comsat_main (0);

  return c != 0;
}

static RETSIGTYPE
sig_hup (int sig)
{
  mu_diag_output (MU_DIAG_NOTICE, _("Restarting"));

  if (xargv[0][0] != '/')
    mu_diag_output (MU_DIAG_ERROR, _("Cannot restart: program must be invoked using absolute pathname"));
  else
    execvp (xargv[0], xargv);

  signal (sig, sig_hup);
}

void
comsat_init ()
{
  /* Register mailbox formats */
  mu_register_all_mbox_formats ();

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
     first three ones, in, out, err.  Do not do the chdir("/").   */
  if (daemon (1, 0) < 0)
    {
      perror (_("Failed to become a daemon"));
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

  fd = socket (PF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    {
      mu_diag_output (MU_DIAG_CRIT, "socket: %m");
      exit (1);
    }

  memset (&local_sin, 0, sizeof local_sin);
  local_sin.sin_family = AF_INET;
  local_sin.sin_addr.s_addr = INADDR_ANY; /*FIXME*/
  local_sin.sin_port = htons (port);

  if (bind (fd, (struct sockaddr *) &local_sin, sizeof local_sin) < 0)
    {
      mu_diag_output (MU_DIAG_CRIT, "bind: %m");
      exit (1);
    }

  mu_diag_output (MU_DIAG_NOTICE, _("GNU comsat started"));

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
	    mu_diag_output (MU_DIAG_ERROR, "select: %m");
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
	      mu_diag_output (MU_DIAG_NOTICE,
		      ngettext ("Too many requests: pausing for %u second",
				"Too many requests: pausing for %u seconds",
				delay),
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
check_connection (int fd, struct sockaddr *addr, socklen_t addrlen)
{
  switch (addr->sa_family)
    {
    case PF_UNIX:
      mu_diag_output (MU_DIAG_INFO, _("connect from socket"));
      break;
      
    case PF_INET:
      {
	struct sockaddr_in *s_in = (struct sockaddr_in *)addr;
	
	if (comsat_acl)
	  {
	    mu_acl_result_t res;
	    
	    int rc = mu_acl_check_sockaddr (comsat_acl, addr, addrlen,
					    &res);
	    if (rc)
	      {
		mu_error (_("Access from %s blocked: cannot check ACLs: %s"),
			  inet_ntoa (s_in->sin_addr), mu_strerror (rc));
		return 1;
	      }
	    switch (res)
	      {
	      case mu_acl_result_undefined:
		mu_diag_output (MU_DIAG_INFO,
				_("%s: undefined ACL result; access allowed"),
				inet_ntoa (s_in->sin_addr));
		break;
		
	      case mu_acl_result_accept:
		break;
		
	      case mu_acl_result_deny:
		mu_error (_("Access from %s blocked."),
			  inet_ntoa (s_in->sin_addr));
		return 1;
	      }
	  }
      }
    }
  return 0;
}

int
comsat_main (int fd)
{
  int rdlen;
  int len;
  struct sockaddr fromaddr;
  char buffer[216]; /*FIXME: Arbitrary size */
  pid_t pid;
  char tty[MAX_TTY_SIZE];
  char *p;
  char *path = NULL;
  mu_message_qid_t qid;
  
  len = sizeof fromaddr;
  rdlen = recvfrom (fd, buffer, sizeof buffer, 0, &fromaddr, &len);
  if (rdlen <= 0)
    {
      if (errno == EINTR)
	return 0;
      mu_diag_output (MU_DIAG_ERROR, "recvfrom: %m");
      return 1;
    }

  if (check_connection (fd, &fromaddr, len))
    return 1;

  mu_diag_output (MU_DIAG_INFO,
		  ngettext ("Received %d byte from %s",
			    "Received %d bytes from %s", rdlen),
		  rdlen, inet_ntoa (((struct sockaddr_in*)&fromaddr)->sin_addr));
  
  buffer[rdlen] = 0;
  mu_diag_output (MU_DIAG_INFO, "string: %s", buffer);

  /* Parse the buffer */
  p = strchr (buffer, '@');
  if (!p)
    {
      mu_diag_output (MU_DIAG_ERROR, _("Malformed input: %s"), buffer);
      return 1;
    }
  *p++ = 0;

  qid = p;
  p = strchr (qid, ':');
  if (p)
    {
      *p++ = 0;
      path = p;
    }
    
  if (find_user (buffer, tty) != SUCCESS)
    return 0;

  /* All I/O is done by child process. This is to avoid various blocking
     problems. */

  pid = fork ();

  if (pid == -1)
    {
      mu_diag_output (MU_DIAG_ERROR, "fork: %m");
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
  notify_user (buffer, tty, path, qid);
  exit (0);
}

static const char *
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
static void
notify_user (const char *user, const char *device, const char *path,
	     mu_message_qid_t qid)
{
  FILE *fp;
  const char *cr;
  mu_mailbox_t mbox = NULL;
  mu_message_t msg;
  int status;

  change_user (user);
  if ((fp = fopen (device, "w")) == NULL)
    {
      mu_error (_("Cannot open device %s: %m"), device);
      exit (0);
    }

  cr = get_newline_str (fp);

  if (!path)
    {
      path = mailbox_path (user);
      if (!path)
	return;
    }

  if ((status = mu_mailbox_create (&mbox, path)) != 0
      || (status = mu_mailbox_open (mbox, MU_STREAM_READ|MU_STREAM_QACCESS)) != 0)
    {
      mu_error (_("Cannot open mailbox %s: %s"),
	      path, mu_strerror (status));
      return;
    }

  status = mu_mailbox_quick_get_message (mbox, qid, &msg);
  if (status)
    {
      mu_error (_("Cannot get message (mailbox %s, qid %s): %s"),
		path, qid, mu_strerror (status));
      return; /* FIXME: Notify the user, anyway */
    }

  run_user_action (fp, cr, msg);
  fclose (fp);
}

/* Search utmp for the local user */
static int
find_user (const char *name, char *tty)
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
	  if (strncmp (ftty, PATH_TTY_PFX, strlen (PATH_TTY_PFX)))
	    {
	      /* An attempt to break security... */
	      mu_diag_output (MU_DIAG_ALERT, _("Bad line name in utmp record: %s"), ftty);
	      return NOT_HERE;
	    }

	  if (stat (ftty, &statb) == 0)
	    {
	      if (!S_ISCHR (statb.st_mode))
		{
		  mu_diag_output (MU_DIAG_ALERT, _("Not a character device: %s"), ftty);
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
change_user (const char *user)
{
  struct passwd *pw;

  pw = getpwnam (user);
  if (!pw)
    {
      mu_diag_output (MU_DIAG_CRIT, _("No such user: %s"), user);
      exit (1);
    }

  setgid (pw->pw_gid);
  setuid (pw->pw_uid);
  chdir (pw->pw_dir);
  username = user;
}

char *
mailbox_path (const char *user)
{
  struct mu_auth_data *auth;
  char *mailbox_name;

  auth = mu_get_auth_by_name (user);

  if (!auth)
    {
      mu_diag_output (MU_DIAG_ALERT, _("User nonexistent: %s"), user);
      return NULL;
    }

  mailbox_name = strdup (auth->mailbox);
  mu_auth_data_free (auth);
  return mailbox_name;
}


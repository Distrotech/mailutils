/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#include "maidag.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <mu_umaxtostr.h>

typedef union
{
  struct sockaddr sa;
  struct sockaddr_in s_in;
  struct sockaddr_un s_un;
} all_addr_t;

static int
lmtp_open_internal (int *pfd, mu_url_t url, const char *urlstr)
{
  int fd;
  int rc;
  int t = 1;
  char buffer[64];
  all_addr_t addr;
  int addrsize;
  mode_t saved_umask;
  
  rc = mu_url_get_scheme (url, buffer, sizeof buffer, NULL);
  if (rc)
    {
      mu_error (_("%s: cannot get scheme from URL: %s"),
		urlstr, mu_strerror(rc));
      return EX_CONFIG;
    }

  memset (&addr, 0, sizeof addr);
  if (strcmp (buffer, "file") == 0 || strcmp (buffer, "socket") == 0)
    {
      size_t size;
      
      rc = mu_url_get_path (url, NULL, 0, &size);
      if (rc)
	{
	  mu_error (_("%s: cannot get path: %s"), urlstr, mu_strerror(rc));
	  return EX_CONFIG;
	}

      if (size > sizeof addr.s_un.sun_path - 1)
	{
	  mu_error (_("%s: file name too long"), urlstr);
	  return EX_TEMPFAIL;
	}
      mu_url_get_path (url, addr.s_un.sun_path, sizeof addr.s_un.sun_path,
		       NULL);

      fd = socket (PF_UNIX, SOCK_STREAM, 0);
      if (fd < 0)
	{
	  mu_error ("socket: %s", mu_strerror (errno));
	  return EX_TEMPFAIL;
	}
  
      addr.s_un.sun_family = AF_UNIX;
      addrsize = sizeof addr.s_un;

      if (reuse_lmtp_address)
	{
	  struct stat st;
	  if (stat (addr.s_un.sun_path, &st))
	    {
	      if (errno != ENOENT)
		{
		  mu_error (_("file %s exists but cannot be stat'd"),
			    addr.s_un.sun_path);
		  return EX_TEMPFAIL;
		}
	    }
	  else if (!S_ISSOCK (st.st_mode))
	    {
	      mu_error (_("file %s is not a socket"),
			addr.s_un.sun_path);
	      return EX_TEMPFAIL;
	    }
	  else
	    unlink (addr.s_un.sun_path);
	}
      
    }
  else if (strcmp (buffer, "tcp") == 0)
    {
      size_t size;
      long n;
      struct hostent *hp;
      char *path = NULL;
      short port = 0;
      
      rc = mu_url_get_port (url, &n);
      if (rc)
	{
	  mu_error (_("%s: cannot get port: %s"), urlstr, mu_strerror(rc));
	  return EX_CONFIG;
	}

      if (n == 0 || (port = n) != n)
	{
	  mu_error (_("Port out of range: %ld"), n);
	  return EX_CONFIG;
	}
			
      rc = mu_url_get_host (url, NULL, 0, &size);
      if (rc)
	{
	  mu_error (_("%s: cannot get host: %s"), urlstr, mu_strerror(rc));
	  return EX_CONFIG;
	}
      path = malloc (size + 1);
      if (!path)
	{
	  mu_error (_("Not enough memory"));
	  return EX_TEMPFAIL;
	}
      mu_url_get_host (url, path, size + 1, NULL);

      fd = socket (PF_INET, SOCK_STREAM, 0);
      if (fd < 0)
	{
	  mu_error ("socket: %s", mu_strerror (errno));
	  return EX_TEMPFAIL;
	}

      addr.s_in.sin_family = AF_INET;
      hp = gethostbyname (path);
      if (hp)
	{
	  char **ap;
	  int count = 0;

	  addr.s_in.sin_addr.s_addr = *(unsigned long*) hp->h_addr_list[0];
	  
	  for (ap = hp->h_addr_list; *ap; ap++)
	    count++;
	  if (count > 1)
	    mu_error (_("warning: %s has several IP addresses, using %s"),
		      path, inet_ntoa (addr.s_in.sin_addr));
	}
      else if (inet_aton (path, &addr.s_in.sin_addr) == 0)
	{
	  mu_error ("invalid IP address: %s", path);
	  return EX_TEMPFAIL;
	}
      addr.s_in.sin_port = htons (port);
      addrsize = sizeof addr.s_in;

      if (reuse_lmtp_address)
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t));
    }
  else
    {
      mu_error (_("%s: invalid scheme"), urlstr);
      return EX_CONFIG;
    }

  saved_umask = umask (0117);
  if (bind (fd, &addr.sa, addrsize) == -1)
    {
      mu_error ("bind: %s", strerror (errno));
      close (fd);
      return EXIT_FAILURE;
    }
  umask (saved_umask);
  *pfd = fd;
  return 0;
}

int
lmtp_open (int *pfd, char *urlstr)
{
  mu_url_t url = NULL;
  int rc;
  
  rc = mu_url_create (&url, urlstr);
  if (rc)
    {
      mu_error (_("%s: cannot create URL: %s"),
		urlstr, mu_strerror (rc));
      return EX_CONFIG;
    }
  rc = mu_url_parse (url);
  if (rc)
    {
      mu_error (_("%s: error parsing URL: %s"),
		urlstr, mu_strerror(rc));
      return EX_CONFIG;
    }

  rc = lmtp_open_internal (pfd, url, urlstr);
  mu_url_destroy (&url);
  return rc;
}

size_t children;
static int need_cleanup = 0;

void
process_cleanup ()
{
  pid_t pid;
  int status;
  
  if (need_cleanup)
    {
      need_cleanup = 0;
      while ( (pid = waitpid (-1, &status, WNOHANG)) > 0)
	--children;
    }
}

RETSIGTYPE
lmtp_sigchld (int signo MU_ARG_UNUSED)
{
  need_cleanup = 1;
#ifndef HAVE_SIGACTION
  signal (signo, lmtp_sigchld);
#endif
}

void
lmtp_reply (FILE *fp, char *code, char *enh, char *fmt, ...)
{
  va_list ap;
  char *str;
  
  va_start (ap, fmt);
  vasprintf (&str, fmt, ap);
  va_end (ap);

  if (daemon_param.transcript)
    {
      if (enh)
	syslog (LOG_INFO, "LMTP reply: %s %s %s", code, enh, str);
      else
	syslog (LOG_INFO, "LMTP reply: %s %s", code, str);
    }
  
  if (!str)
    {
      mu_error (_("Not enough memory"));
      exit (EX_TEMPFAIL);
    }

  while (*str)
    {
      char *end = strchr (str, '\n');

      if (end)
	{
	  size_t len = end - str;
	  fprintf (fp, "%s-", code);
	  if (enh)
	    fprintf (fp, "%s ", enh);
	  fprintf (fp, "%.*s\r\n", len, str);
	  for (str = end; *str && *str == '\n'; str++);
	}
      else
	{
	  fprintf (fp, "%s ", code);
	  if (enh)
	    fprintf (fp, "%s ", enh);
	  fprintf (fp, "%s\r\n", str);
	  str += strlen (str);
	}
    }
}

void
trimnl (char *arg)
{
  size_t len = strlen (arg);
  if (len > 0 && arg[len-1] == '\n')
    arg[--len] = 0;
  if (len > 0 && arg[len-1] == '\r')
    arg[--len] = 0;
}

void
xlatnl (char *arg)
{
  size_t len = strlen (arg);
  if (len > 0 && arg[len-1] == '\n')
    {
      len--;
      if (len > 0 && arg[len-1] == '\r')
	{
	  arg[len-1] = '\n';
	  arg[len] = 0;
	}
    }
}


enum lmtp_state
  {
    state_none,
    
    state_init,    
    state_lhlo,    
    state_mail,    
    state_rcpt,    
    state_data,    
    state_quit,    
    state_dot,

    state_end
  };

#define NSTATE ((int) state_end + 1)

enum lmtp_command
  {
    cmd_unknown,
    cmd_lhlo,
    cmd_mail,    
    cmd_rcpt,    
    cmd_data,
    cmd_quit,
    cmd_rset,
    cmd_help,
    cmd_dot
  };

#define NCMD ((int)cmd_dot + 1)

#define SNO state_none
#define SIN state_init
#define SHL state_lhlo
#define SML state_mail     
#define SRC state_rcpt     
#define SDA state_data     
#define SQT state_quit     
#define SDT state_dot     
#define SEN state_end

int transtab[NCMD][NSTATE] = {
/* state_     SNO  SIN  SHL  SML  SRC  SDA  SQT  SDT  SEN */
/* unkn */  { SNO, SNO, SNO, SNO, SNO, SNO, SNO, SNO, SEN },
/* lhlo */  { SNO, SHL, SNO, SNO, SNO, SNO, SNO, SNO, SNO },
/* mail */  { SNO, SNO, SML, SNO, SNO, SNO, SNO, SNO, SNO },
/* rcpt */  { SNO, SNO, SNO, SRC, SRC, SNO, SNO, SNO, SNO },
/* data */  { SNO, SNO, SNO, SNO, SDA, SNO, SNO, SNO, SNO },
/* quit */  { SNO, SEN, SEN, SEN, SEN, SEN, SEN, SEN, SEN },
/* rset */  { SNO, SIN, SIN, SIN, SIN, SIN, SIN, SIN, SNO },
/* help */  { SNO, SIN, SHL, SML, SRC, SDT, SQT, SDT, SEN },
/* dot  */  { SNO, SNO, SNO, SNO, SNO, SQT, SNO, SNO, SNO },
};


/* Delivery data */
char *lhlo_domain;     /* Sender domain */
char *mail_from;       /* Sender address */
mu_list_t rcpt_list;   /* Recipient addresses */
struct mail_tmp *mtmp; /* Temporary mail storage */
mu_mailbox_t mbox;     /* Collected mail body */

static void
rcpt_to_destroy_item (void *ptr)
{
  free (ptr);
}


int
cfun_unknown (FILE *out, char *arg)
{
  lmtp_reply (out, "500", "5.5.1", "Command unrecognized");
  return 0;
}


static void
add_default_domain (char *str, int len, char **pret)
{
  *pret = malloc (len + 1 + strlen (lhlo_domain) + 1);
  if (!*pret)
    {
      mu_error (_("Not enough memory"));
      exit (EX_SOFTWARE);
    }
  memcpy (*pret, str, len);
  (*pret)[len] = '@';
  strcpy (*pret + len + 1, lhlo_domain);
}

#define MAILER_DAEMON "MAILER-DAEMON"

int
check_address (char *arg, int with_domain, char **pret)
{
  if (strchr (arg, '@') == 0)
    {
      char *addr = NULL;
      size_t addrlen = 0;
      
      if (*arg == '<')
	{
	  size_t len = strlen (arg);
	  if (arg[len - 1] == '>')
	    {
	      if (len == 2) /* null address */
		{
		  if (!with_domain)
		    /* Null address is only legal in mail from */
		    return 1;
		  addr = MAILER_DAEMON;
		  addrlen = sizeof MAILER_DAEMON - 1;
		}
	      else
		{
		  addr = arg + 1;
		  addrlen = len - 2;
		}
	    }
	  else
	    return 1;
	}
      else
	{
	  addr = arg;
	  addrlen = strlen (arg);
	}

      if (with_domain)
	add_default_domain (addr, addrlen, pret);
      else
	{
	  *pret = malloc (addrlen + 1);
	  memcpy (*pret, addr, addrlen);
	  (*pret)[addrlen] = 0;
	}
    }
  else
    {
      mu_address_t addr;
      int rc = mu_address_create (&addr, arg);
      if (rc)
	return 1;
      if (with_domain)
	mu_address_aget_email (addr, 1, pret);
      else
	mu_address_aget_local_part (addr, 1, pret);
      mu_address_destroy (&addr);
    }
  return 0;
}


int
cfun_mail_from (FILE *out, char *arg)
{
  if (*arg == 0)
    {
      lmtp_reply (out, "501", "5.5.2", "Syntax error");
      return 1;
    }

  if (check_address (arg, 1, &mail_from))
    {
      lmtp_reply (out, "553", "5.1.8", "Address format error");
      return 1;
    }
  lmtp_reply (out, "250", "2.1.0", "Go ahead");
  return 0;
}


int
cfun_rcpt_to (FILE *out, char *arg)
{
  char *user;
  struct mu_auth_data *auth;
  
  if (*arg == 0)
    {
      lmtp_reply (out, "501", "5.5.2", "Syntax error");
      return 1;
    }

  /* FIXME: Check if domain is OK */
  if (check_address (arg, 0, &user))
    {
      lmtp_reply (out, "553", "5.1.8", "Address format error");
      return 1;
    }
  auth = mu_get_auth_by_name (user);
  if (!auth)
    {
      lmtp_reply (out, "550", "5.1.1", "User unknown");
      free (user);
      return 1;
    }
  mu_auth_data_free (auth);
  if (!rcpt_list)
    {
      mu_list_create (&rcpt_list);
      mu_list_set_destroy_item (rcpt_list, rcpt_to_destroy_item);
    }
  mu_list_append (rcpt_list, user);
  lmtp_reply (out, "250", "2.1.5", "Go ahead");
  return 0;
}  


int
cfun_data (FILE *out, char *arg)
{
  if (*arg)
    {
      lmtp_reply (out, "501", "5.5.2", "Syntax error");
      return 1;
    }

  if (mail_tmp_begin (&mtmp, mail_from))
    {
      /* FIXME: codes */
      lmtp_reply (out, "450", "4.1.0", "Temporary failure, try again later");
      return 1;
    }
  lmtp_reply (out, "354", NULL, "Go ahead");
  return 0;
}


int
dot_temp_fail (void *item, void *cbdata)
{
  char *name = item;
  FILE *out = cbdata;
  lmtp_reply (out, "450", "4.1.0", "%s: temporary failure", name);
  return 0;
}

int
dot_deliver (void *item, void *cbdata)
{
  char *name = item;
  FILE *out = cbdata;
  char *errp = NULL;
  
  switch (deliver (mbox, name, &errp))
    {
    case 0:
      lmtp_reply (out, "250", "2.0.0", "%s: delivered", name);
      break;

    case EX_UNAVAILABLE:
      if (errp)
	lmtp_reply (out, "553", "5.1.8", "%s", errp);
      else
	lmtp_reply (out, "553", "5.1.8", "%s: delivery failed", name);
      break;

    default:
      if (errp)
	lmtp_reply (out, "450", "4.1.0", "%s", errp);
      else
	lmtp_reply (out, "450", "4.1.0",
		    "%s: temporary failure, try again later",
		    name);
      break;
    }
  free (errp);
  return 0;
}

int
cfun_dot (FILE *out, char *arg)
{
  if (!mtmp)
    mu_list_do (rcpt_list, dot_temp_fail, out);
  else
    {
      int rc = mail_tmp_finish (mtmp, &mbox);
      if (rc)
	mu_list_do (rcpt_list, dot_temp_fail, out);
      else
	{
	  mu_list_do (rcpt_list, dot_deliver, out);
	  mail_tmp_destroy (&mtmp);
	  mu_mailbox_destroy (&mbox);
	}
    }
  free (mail_from);
  mu_list_destroy (&rcpt_list);
  return 0;
}
  

int
cfun_rset (FILE *out, char *arg)
{
  free (lhlo_domain);
  free (mail_from);
  mu_list_destroy (&rcpt_list);
  mail_tmp_destroy (&mtmp);
  mu_mailbox_destroy (&mbox);
  lmtp_reply (out, "250", "2.0.0", "OK, forgotten");
  return 0;
}


char *capa_str = "ENHANCEDSTATUSCODES\n\
PIPELINING\n\
8BITMIME\n\
HELP";

int
cfun_lhlo (FILE *out, char *arg)
{
  if (*arg == 0)
    {
      lmtp_reply (out, "501", "5.0.0", "Syntax error");
      return 1;
    }
  lhlo_domain = strdup (arg);
  lmtp_reply (out, "250", NULL, "Hello\n");
  lmtp_reply (out, "250", NULL, capa_str);
  return 0;
}


int
cfun_quit (FILE *out, char *arg)
{
  lmtp_reply (out, "221", "2.0.0", "Bye");
  return 0;
}


int
cfun_help (FILE *out, char *arg)
{
  lmtp_reply (out, "200", "2.0.0", "Man, help yourself");
  return 0;
}


struct command_tab
{
  char *cmd_verb;
  int cmd_len;
  enum lmtp_command cmd_code;
  int (*cmd_fun) (FILE *, char *);
} command_tab[] = {
#define S(s) #s, (sizeof #s - 1)
  { S(lhlo), cmd_lhlo, cfun_lhlo },
  { S(mail from:), cmd_mail, cfun_mail_from },   
  { S(rcpt to:), cmd_rcpt, cfun_rcpt_to },   
  { S(data), cmd_data, cfun_data },
  { S(quit), cmd_quit, cfun_quit },
  { S(rset), cmd_rset, cfun_rset },
  { S(help), cmd_help, cfun_help },
  { S(.), cmd_dot, cfun_dot },
  { NULL, 0, cmd_unknown, cfun_unknown }
};

struct command_tab *
getcmd (char *buf, char **sp)
{
  struct command_tab *cp;
  size_t len = strlen (buf);
  for (cp = command_tab; cp->cmd_verb; cp++)
    {
      if (cp->cmd_len <= len
	  && strncasecmp (cp->cmd_verb, buf, cp->cmd_len) == 0)
	{
	  *sp = buf + cp->cmd_len;
	  return cp;
	}
    }
  return cp;
}

int
lmtp_loop (FILE *in, FILE *out)
{
  char buf[1024];
  enum lmtp_state state = state_init;

  setvbuf (in, NULL, _IOLBF, 0);
  setvbuf (out, NULL, _IOLBF, 0);

  lmtp_reply (out, "220", NULL, "At your service");
  while (fgets (buf, sizeof buf, in))
    {
      if (state == state_data
	  && !(buf[0] == '.'
	       && (buf[1] == '\n' || (buf[1] == '\r' && buf[2] == '\n'))))
	{
	  /* This is a special case */
	  if (mtmp)
	    {
	      size_t len;
	      int rc;

	      xlatnl (buf);
	      len = strlen (buf);
	      if ((rc = mail_tmp_add_line (mtmp, buf, len)))
		{
		  mail_tmp_destroy (&mtmp);
		  /* Wait the dot to report the error */
		}
	    }
	}
      else
	{
	  char *sp;
	  struct command_tab *cp = getcmd (buf, &sp);
	  enum lmtp_command cmd = cp->cmd_code;
	  enum lmtp_state next_state = transtab[cmd][state];

	  trimnl (buf);

	  if (daemon_param.transcript)
	    syslog (LOG_INFO, "LMTP recieve: %s", buf);
	      
	  if (next_state != state_none)
	    {
	      if (cp->cmd_fun)
		{
		  while (*sp && isspace (*sp))
		    sp++;
		  if (cp->cmd_fun (out, sp))
		    continue;
		}
	      state = next_state;
	    }
	  else
	    lmtp_reply (out, "503", "5.0.0", "Syntax error");
	}
      
      if (state == state_end)
	break;
    }
  return 0;
}

void
log_connection (all_addr_t *addr, socklen_t addrlen)
{
  switch (addr->sa.sa_family)
    {
    case PF_UNIX:
      syslog (LOG_INFO, _("connect from socket"));
      break;
      
    case PF_INET:
      syslog (LOG_INFO, _("connect from %s"), inet_ntoa (addr->s_in.sin_addr));
    }
}

int
lmtp_daemon (char *urlstr)
{
  int rc;
  int listenfd, connfd;
  all_addr_t addr;
  socklen_t addrlen;
  pid_t pid;
  
  if (daemon_param.mode == MODE_DAEMON)
    {
      if (daemon (0, 0) < 0)
	{
	  mu_error (_("Failed to become a daemon"));
	  return EX_UNAVAILABLE;
	}
    }

  rc = lmtp_open (&listenfd, urlstr);
  if (rc)
    return rc;
  
  if (listen (listenfd, 128) == -1)
    {
      mu_error ("listen: %s", strerror (errno));
      close (listenfd);
      return EX_UNAVAILABLE;
    }

#ifdef HAVE_SIGACTION
  {
    struct sigaction act;
    act.sa_handler = lmtp_sigchld;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGCHLD, &act, NULL);
  }
#else
  signal (SIGCHLD, lmtp_sigchld);
#endif
  
  for (;;)
    {
      process_cleanup ();
      if (children > daemon_param.maxchildren)
	{
	  mu_error (_("too many children (%lu)"),
		    (unsigned long) children);
	  pause ();
	  continue;
	}
      addrlen = sizeof addr;
      connfd = accept (listenfd, (struct sockaddr *)&addr, &addrlen);
      if (connfd == -1)
	{
	  if (errno == EINTR)
	    continue;
	  mu_error ("accept: %s", strerror (errno));
	  continue;
	  /*exit (EXIT_FAILURE);*/
	}

      log_connection (&addr, addrlen);
      
      pid = fork ();
      if (pid == -1)
	syslog (LOG_ERR, "fork: %s", strerror (errno));
      else if (pid == 0) /* Child.  */
	{
	  int status;
	  
	  close (listenfd);
	  status = lmtp_loop (fdopen (connfd, "r"), fdopen (connfd, "w"));
	  exit (status);
	}
      else
	{
	  ++children;
	}
      close (connfd);
    }
}

#define DEFAULT_URL "tcp://127.0.0.1:"

int
maidag_lmtp_server ()
{
  struct group *gr = getgrnam (lmtp_group);

  if (gr == NULL)
    {
      mu_error (_("Error getting mail group"));
      return EX_UNAVAILABLE;
    }
  
  if (setgid (gr->gr_gid) == -1)
    {
      mu_error (_("Error setting mail group"));
      return EX_UNAVAILABLE;
    }

  if (lmtp_url_string)
    return lmtp_daemon (lmtp_url_string);
  else if (daemon_param.mode == MODE_INTERACTIVE)
    return lmtp_loop (stdin, stdout);
  else
    {
      const char *pstr = mu_umaxtostr (0, daemon_param.port);
      char *urls = malloc (sizeof (DEFAULT_URL) + strlen (pstr));
      if (!urls)
	{
	  mu_error (_("Not enough memory"));
	  return EX_TEMPFAIL;
	}
      strcpy (urls, DEFAULT_URL);
      strcat (urls, pstr);
      return lmtp_daemon (urls);
    }
}

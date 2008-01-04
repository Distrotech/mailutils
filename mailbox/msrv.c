/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2008 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; If not, see
   <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <ctype.h>
#include <mailutils/server.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/cfg.h>
#include <mailutils/nls.h>
#include <mailutils/daemon.h>
#include <mailutils/acl.h>

typedef RETSIGTYPE (*mu_sig_handler_t) (int);

static mu_sig_handler_t
set_signal (int sig, mu_sig_handler_t handler)
{
#ifdef HAVE_SIGACTION
  {
    struct sigaction act, oldact;
    act.sa_handler = handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (sig, &act, &oldact);
    return oldact.sa_handler;
  }
#else
  return signal (sig, handler);
#endif
}

#ifndef NSIG
# define NSIG 64
#endif

struct _mu_m_server
{
  char *ident;
  mu_server_t server;
  mu_list_t srvlist;
  mu_m_server_conn_fp conn;
  void *data;
  int mode;
  int foreground;
  size_t max_children;
  char *pidfile;
  unsigned short defport;
  time_t timeout;
  mu_acl_t acl;

  sigset_t sigmask;
  mu_sig_handler_t sigtab[NSIG]; 
};

static int need_cleanup = 0;
static int stop = 0;
static size_t children;

static int
mu_m_server_idle (void *server_data MU_ARG_UNUSED)
{
  pid_t pid;
  int status;
  
  if (need_cleanup)
    {
      need_cleanup = 0;
      while ( (pid = waitpid (-1, &status, WNOHANG)) > 0)
	{
	  --children;
	  if (WIFEXITED (status))
	    {
	      int prio = MU_DIAG_INFO;
	      
	      status = WEXITSTATUS (status);
	      if (status == 0)
		prio = MU_DIAG_DEBUG;
	      mu_diag_output (prio, "process %lu finished with code %d",
			      (unsigned long) pid,
			      status);
	    }
	  else if (WIFSIGNALED (status))
	    mu_diag_output (MU_DIAG_ERR, "process %lu terminated on signal %d",
			    (unsigned long) pid,
			    WTERMSIG (status));
	  else
	    mu_diag_output (MU_DIAG_ERR,
			    "process %lu terminated (cause unknown)",
			    (unsigned long) pid);
	}
    }
  return stop;
}

static RETSIGTYPE
m_srv_signal (int signo)
{
  switch (signo)
    {
    case SIGCHLD:
      need_cleanup = 1;
      break;

    default:
      stop = 1;
      break;
    }
#ifndef HAVE_SIGACTION
  signal (signo, m_srv_sigchld);
#endif
}

void
mu_m_server_create (mu_m_server_t *psrv, const char *ident)
{
  mu_m_server_t srv = calloc (1, sizeof *srv);
  if (!srv)
    {
      mu_error ("%s", mu_strerror (ENOMEM));
      exit (1);
    }
  if (ident)
    {
      srv->ident = strdup (ident);
      if (!srv->ident)
	{
	  mu_error ("%s", mu_strerror (ENOMEM));
	  exit (1);
	}
    }
  MU_ASSERT (mu_server_create (&srv->server));
  mu_server_set_idle (srv->server, mu_m_server_idle);
  sigemptyset (&srv->sigmask);
  sigaddset (&srv->sigmask, SIGCHLD);
  sigaddset (&srv->sigmask, SIGINT);
  sigaddset (&srv->sigmask, SIGTERM);
  sigaddset (&srv->sigmask, SIGQUIT);
  sigaddset (&srv->sigmask, SIGHUP);
  *psrv = srv;
}

void
mu_m_server_set_sigset (mu_m_server_t srv, sigset_t *sigset)
{
  srv->sigmask = *sigset;
  sigaddset (&srv->sigmask, SIGCHLD);
}

void
mu_m_server_get_sigset (mu_m_server_t srv, sigset_t *sigset)
{
  *sigset = srv->sigmask;
}

void
mu_m_server_set_mode (mu_m_server_t srv, int mode)
{
  srv->mode = mode;
}

void
mu_m_server_set_conn (mu_m_server_t srv, mu_m_server_conn_fp conn)
{
  srv->conn = conn;
}

void
mu_m_server_set_data (mu_m_server_t srv, void *data)
{
  srv->data = data;
}

void
mu_m_server_set_max_children (mu_m_server_t srv, size_t num)
{
  srv->max_children = num;
}

int
mu_m_server_set_pidfile (mu_m_server_t srv, const char *pidfile)
{
  free (srv->pidfile);
  srv->pidfile = strdup (pidfile);
  return srv->pidfile ? 0 : errno;
}

void
mu_m_server_set_default_port (mu_m_server_t srv, int port)
{
  srv->defport = port;
}

void
mu_m_server_set_timeout (mu_m_server_t srv, time_t t)
{
  srv->timeout = t;
}

int
mu_m_server_mode (mu_m_server_t srv)
{
  return srv->mode;
}

time_t
mu_m_server_timeout (mu_m_server_t srv)
{
  return srv->timeout;
}

struct m_srv_config
{
  mu_m_server_t msrv;
  mu_tcp_server_t tcpsrv;
  mu_acl_t acl;
  int single_process;
  int transcript;
  time_t timeout;
};

void
m_srv_config_free (void *data)
{
  struct m_srv_config *pconf = data;
  /* FIXME */
  free (pconf);
}

static int m_srv_conn (int fd, struct sockaddr *sa, int salen,
		       void *server_data, void *call_data,
		       mu_tcp_server_t srv);

static struct m_srv_config *
add_server (mu_m_server_t msrv, struct sockaddr *s, int slen)
{
  mu_tcp_server_t tcpsrv;
  struct m_srv_config *pconf;

  MU_ASSERT (mu_tcp_server_create (&tcpsrv, s, slen));
  MU_ASSERT (mu_tcp_server_set_conn (tcpsrv, m_srv_conn));
  pconf = calloc (1, sizeof (*pconf));
  if (!pconf)
    {
      mu_error ("%s", mu_strerror (ENOMEM));
      exit (1);
    }
  pconf->msrv = msrv;
  pconf->tcpsrv = tcpsrv;
  pconf->single_process = 0;
  pconf->timeout = msrv->timeout;
  MU_ASSERT (mu_tcp_server_set_data (tcpsrv, pconf, m_srv_config_free));
  if (!msrv->srvlist)
    MU_ASSERT (mu_list_create (&msrv->srvlist));
  MU_ASSERT (mu_list_append (msrv->srvlist, tcpsrv));
  return pconf;
}

void
mu_m_server_configured_count (mu_m_server_t msrv, size_t count)
{
  mu_list_count (msrv->srvlist, &count);
}

void
mu_m_server_begin (mu_m_server_t msrv)
{
  int i, rc;
  size_t count = 0;

  mu_list_count (msrv->srvlist, &count);
  if (count == 0 && msrv->defport)
    {
      /* Add default server */
      struct sockaddr_in s;
      s.sin_addr.s_addr = htonl (INADDR_ANY);
      s.sin_port = htons (msrv->defport);
      add_server (msrv, (struct sockaddr *)&s, sizeof s);	  
    }
  
  if (!msrv->foreground)
    {
      /* Become a daemon. Take care to close inherited fds and to hold
	 first three one, in, out, err   */
      if (daemon (0, 0) < 0)
	{
	  mu_error (_("Failed to become a daemon: %s"), mu_strerror (errno));
	  exit (EXIT_FAILURE);
	}
    }

  if (msrv->pidfile)
    switch (rc = mu_daemon_create_pidfile (msrv->pidfile))
      {
      case EINVAL:
	mu_error (_("%s: Invalid name for a pidfile"), msrv->pidfile);
	break;
	
      default:
	mu_error (_("cannot create pidfile `%s': %s"), msrv->pidfile,
		  mu_strerror (rc));
      }

  for (i = 0; i < NSIG; i++)
    if (sigismember (&msrv->sigmask, i))
      msrv->sigtab[i] = set_signal (i, m_srv_signal);
}

void
mu_m_server_end (mu_m_server_t msrv)
{
  int i;
  
  for (i = 0; i < NSIG; i++)
    if (sigismember (&msrv->sigmask, i))
      set_signal (i, msrv->sigtab[i]);
}

void
mu_m_server_destroy (mu_m_server_t *pmsrv)
{
  mu_m_server_t msrv = *pmsrv;
  mu_server_destroy (&msrv->server);
  /* FIXME: Send processes the TERM signal */
  free (msrv->ident);
  free (msrv);
  *pmsrv = NULL;
}

static int
tcp_conn_handler (int fd, void *conn_data, void *server_data)
{
  mu_tcp_server_t tcpsrv = (mu_tcp_server_t) conn_data;
  int rc = mu_tcp_server_accept (tcpsrv, server_data);
  if (rc && rc != EINTR)
    {
      mu_tcp_server_shutdown (tcpsrv);
      return MU_SERVER_CLOSE_CONN;
    }
  return stop ? MU_SERVER_SHUTDOWN : MU_SERVER_SUCCESS;
}

static void
tcp_conn_free (void *conn_data, void *server_data)
{
  mu_tcp_server_t tcpsrv = (mu_tcp_server_t) conn_data;
  mu_tcp_server_destroy (&tcpsrv);
}

static int
_open_conn (void *item, void *data)
{
  union
  {
    struct sockaddr sa;
    char pad[512];
  }
  addr;
  int addrlen = sizeof addr;
  char *p;
  mu_tcp_server_t tcpsrv = item;
  mu_m_server_t msrv = data;
  int rc = mu_tcp_server_open (tcpsrv);
  if (rc)
    {
      mu_tcp_server_get_sockaddr (tcpsrv, &addr.sa, &addrlen);
      p = mu_sockaddr_to_astr (&addr.sa, addrlen);
      mu_error (_("Cannot open connection on %s: %s"), p, mu_strerror (rc));
      free (p);
      return 0;
    }
  rc = mu_server_add_connection (msrv->server,
				 mu_tcp_server_get_fd (tcpsrv),
				 tcpsrv,
				 tcp_conn_handler, tcp_conn_free);
  if (rc)
    {
      mu_tcp_server_get_sockaddr (tcpsrv, &addr.sa, &addrlen);
      p = mu_sockaddr_to_astr (&addr.sa, addrlen);
      mu_error (_("Cannot add connection %s: %s"), p, mu_strerror (rc));
      free (p);
      mu_tcp_server_shutdown (tcpsrv);
      mu_tcp_server_destroy (&tcpsrv);
    }
  return 0;
}  

int
mu_m_server_run (mu_m_server_t msrv)
{
  int rc;
  size_t count;
  mode_t saved_umask = umask (0117);
  mu_list_do (msrv->srvlist, _open_conn, msrv);
  umask (saved_umask);
  mu_list_destroy (&msrv->srvlist);
  MU_ASSERT (mu_server_count (msrv->server, &count));
  if (count == 0)
    {
      mu_error (_("No servers configured: exiting"));
      exit (1);
    }
  if (msrv->ident)
    mu_diag_output (MU_DIAG_INFO, _("%s started"), msrv->ident);
  rc = mu_server_run (msrv->server);
  if (msrv->ident)
    mu_diag_output (MU_DIAG_INFO, _("%s terminated"), msrv->ident);
  return rc;
}



static int
check_global_acl (mu_m_server_t msrv, struct sockaddr *s, int salen)
{
  if (msrv->acl)
    {
      mu_acl_result_t res;
      int rc = mu_acl_check_sockaddr (msrv->acl, s, salen, &res);
      if (rc)
	{
	  char *p = mu_sockaddr_to_astr (s, salen);
	  mu_error (_("Access from %s blocked: cannot check ACLs: %s"),
		    p, mu_strerror (rc));
	  free (p);
	  return 1;
	}
      switch (res)
	{
	case mu_acl_result_undefined:
	  {
	    char *p = mu_sockaddr_to_astr (s, salen);
	    mu_diag_output (MU_DIAG_INFO,
			    _("%s: undefined ACL result; access allowed"),
			    p);
	    free (p);
	  }
	  break;
	      
	case mu_acl_result_accept:
	  break;
	      
	case mu_acl_result_deny:
	  {
	    char *p = mu_sockaddr_to_astr (s, salen);
	    mu_error (_("Access from %s blocked."), p);
	    free (p);
	    return 1;
	  }
	}
    }
  return 0;
}

int
m_srv_conn (int fd, struct sockaddr *sa, int salen,
	    void *server_data, void *call_data,
	    mu_tcp_server_t srv)
{
  int status;
  struct m_srv_config *pconf = server_data;

  if (check_global_acl (pconf->msrv, sa, salen))
    return 0;
  
  if (!pconf->single_process)
    {
      pid_t pid;

      if (mu_m_server_idle (server_data))
	return MU_SERVER_SHUTDOWN;
      if (children >= pconf->msrv->max_children)
        {
	  mu_diag_output (MU_DIAG_ERROR, _("too many children (%lu)"),
			  (unsigned long) children);
          pause ();
          return 0;
        }

      pid = fork ();
      if (pid == -1)
	mu_diag_output (MU_DIAG_ERROR, "fork: %s", strerror (errno));
      else if (pid == 0) /* Child.  */
	{
	  mu_tcp_server_shutdown (srv);
	  status = pconf->msrv->conn (fd, pconf->msrv->data,
				      pconf->timeout, pconf->transcript);
	  closelog ();
	  exit (status);
	}
      else
	{
	  children++;
	}
    }
  else
    pconf->msrv->conn (fd, pconf->msrv->data,
		       pconf->timeout, pconf->transcript);
  
  return 0;
}



unsigned short
get_port (mu_debug_t debug, char *p)
{
  if (p)
    {
      char *q;
      unsigned long n = strtoul (p, &q, 0);
      if (*q == 0)
	{
	  if (n > USHRT_MAX)
	    {
	      mu_debug_printf (debug, MU_DEBUG_ERROR,
				   _("invalid port number: %s\n"), p);
	      return 1;
	    }
	  
	  return htons (n);
	}
      else
	{
	  struct servent *sp = getservbyname (p, "tcp");
	  if (!sp)
	    return 0;
	  return sp->s_port;
	}
    }
  return 0;
}

static int
get_family (char **pstr, sa_family_t *pfamily)
{
  static struct family_tab
  {
    int len;
    char *pfx;
    int family;
  } ftab[] = {
#define S(s,f) { sizeof (#s":") - 1, #s":", f }
    S (file, AF_UNIX),
    S (unix, AF_UNIX),
    S (local, AF_UNIX),
    S (socket, AF_UNIX),
    S (inet, AF_INET),
    S (tcp, AF_INET),
#undef S
    { 0 }
  };
  struct family_tab *fp;
  
  char *str = *pstr;
  int len = strlen (str);
  for (fp = ftab; fp->len; fp++)
    {
      if (len > fp->len && memcmp (str, fp->pfx, fp->len) == 0)
	{
	  str += fp->len;
	  if (str[0] == '/' && str[1] == '/')
	    str += 2;
	  *pstr = str;
	  *pfamily = fp->family;
	  return 0;
	}
    }
  return 1;
}

static int
is_ip_addr (const char *arg)
{
  int     dot_count;
  int     digit_count;

  dot_count = 0;
  digit_count = 0;
  for (; *arg != 0 && *arg != ':'; arg++)
    {
      if (*arg == '.')
	{
	  if (++dot_count > 3)
	    break;
	  digit_count = 0;
	}
      else if (!(isdigit (*arg) && ++digit_count <= 3))
	return 0;
    }
  return dot_count == 3;
}  

static int
server_block_begin (mu_debug_t debug, char *arg, mu_m_server_t msrv,
		    void **pdata)
{
  char *p;
  union
  {
    struct sockaddr s_sa;
    struct sockaddr_in s_in;
    struct sockaddr_un s_un;
  }
  s;
  int salen = sizeof s.s_in;
  
  s.s_sa.sa_family = AF_INET;
  
  if (!arg)
    {
      s.s_in.sin_addr.s_addr = htonl (INADDR_ANY);
      s.s_in.sin_port = htons (msrv->defport);
    }
  else
    {
      unsigned short n;
      int len;
      
      if (!is_ip_addr (arg) && get_family (&arg, &s.s_sa.sa_family))
	{
	  mu_debug_printf (debug, MU_DEBUG_ERROR, _("invalid family\n"));
	  return 1;
	}
      
      switch (s.s_in.sin_family)
	{
	case AF_INET:
	  if ((n = get_port (debug, arg)))
	    {
	      s.s_in.sin_addr.s_addr = htonl (INADDR_ANY);
	      s.s_in.sin_port = htons (n);	  
	    }
	  else
	    {
	      p = strchr (arg, ':');
	      if (p)
		*p++ = 0;
	      if (inet_aton (arg, &s.s_in.sin_addr) == 0)
		{
		  struct hostent *hp = gethostbyname (arg);
		  if (hp)
		    s.s_in.sin_addr.s_addr = *(unsigned long *)hp->h_addr;
		  else
		    {
		      mu_debug_printf (debug, MU_DEBUG_ERROR,
				       _("invalid IP address: %s\n"), arg);
		      return 1;
		    }
		}
	      if (p)
		{
		  n = get_port (debug, p);
		  if (!n)
		    {
		      mu_debug_printf (debug, MU_DEBUG_ERROR,
				       _("invalid port number: %s\n"), p);
		      return 1;
		    }
		  s.s_in.sin_port = n;
		}
	      else if (msrv->defport)
		s.s_in.sin_port = htons (msrv->defport);
	      else
		{
		  mu_debug_printf (debug, MU_DEBUG_ERROR,
				   _("missing port number\n"));
		  return 1;
		}
	    }
	  break;

	case AF_UNIX:
	  salen = sizeof s.s_un;
	  len = strlen (arg);
	  if (len > sizeof s.s_un.sun_path - 1)
	    {
	      mu_error (_("%s: file name too long"), arg);
	      return 1;
	    }
	  strcpy (s.s_un.sun_path, arg);
	  break;
	}
    }
      
  *pdata = add_server (msrv, &s.s_sa, salen);
  return 0;
}

static int
server_section_parser (enum mu_cfg_section_stage stage,
		       const mu_cfg_node_t *node,
		       const char *section_label, void **section_data,
		       void *call_data,
		       mu_cfg_tree_t *tree)
{
  switch (stage)
    {
    case mu_cfg_section_start:
      {
	/* FIXME: should not modify 2nd arg, or it should not be const */
	return server_block_begin (tree->debug, node->tag_label,
				   *section_data, section_data);
      }
      break;

    case mu_cfg_section_end:
      {
	struct m_srv_config *pconf = *section_data;
	if (pconf->acl)
	  mu_tcp_server_set_acl (pconf->tcpsrv, pconf->acl);
      }
      break;
    }
  return 0;
}

static int
_cb_daemon_mode (mu_debug_t debug, void *data, char *arg)
{
  int *pmode = data;
  
  if (strcmp (arg, "inetd") == 0
      || strcmp (arg, "interactive") == 0)
    *pmode = MODE_INTERACTIVE;
  else if (strcmp (arg, "daemon") == 0)
    *pmode = MODE_DAEMON;
  else
    {
      mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("unknown daemon mode"));
      return 1;
    }
  return 0;
}

static struct mu_cfg_param dot_server_cfg_param[] = {
  { "max-children", mu_cfg_size,
    NULL, mu_offsetof (struct _mu_m_server,max_children), NULL,
    N_("Maximum number of children processes to run simultaneously.") },
  { "mode", mu_cfg_callback,
    NULL, mu_offsetof (struct _mu_m_server,mode), _cb_daemon_mode,
    N_("Set daemon mode (either inetd (or interactive) or daemon)."),
    N_("mode") },
  { "foreground", mu_cfg_bool,
    NULL, mu_offsetof (struct _mu_m_server, foreground), NULL,
    N_("Run in foreground.") },
  { "pidfile", mu_cfg_string,
    NULL, mu_offsetof (struct _mu_m_server,pidfile), NULL,
    N_("Store PID of the master process in this file."),
    N_("file") },
  { "port", mu_cfg_ushort,
    NULL, mu_offsetof (struct _mu_m_server,defport), NULL,
    N_("Default port number.") },
  { "timeout", mu_cfg_time,
    NULL, mu_offsetof (struct _mu_m_server,timeout), NULL,
    N_("Set idle timeout.") },
  { "server", mu_cfg_section, NULL, 0, NULL,
    N_("Server configuration.") },
  { "acl", mu_cfg_section, NULL, mu_offsetof (struct _mu_m_server,acl), NULL,
    N_("Per-server access control list") },
  { NULL }
};
    
static struct mu_cfg_param server_cfg_param[] = {
  { "single-process", mu_cfg_bool, 
    NULL, mu_offsetof (struct m_srv_config, single_process), NULL,
    N_("Run this server in foreground.") },
  { "transcript", mu_cfg_bool,
    NULL, mu_offsetof (struct m_srv_config, transcript), NULL,
    N_("Log the session transcript.") },
  { "timeout", mu_cfg_time,
    NULL, mu_offsetof (struct m_srv_config, timeout), NULL,
    N_("Set idle timeout.") },
  { "acl", mu_cfg_section,
    NULL, mu_offsetof (struct m_srv_config, acl), NULL,
    N_("Global access control list.") },
  { NULL }
};

void
mu_m_server_cfg_init ()
{
  struct mu_cfg_section *section;
  if (mu_create_canned_section ("server", &section) == 0)
    {
      section->parser = server_section_parser;
      section->label = N_("ipaddr[:port]");
      mu_cfg_section_add_params (section, server_cfg_param);
    }
  if (mu_create_canned_section (".server", &section) == 0)
    {
      mu_cfg_section_add_params (section, dot_server_cfg_param);
    }
}




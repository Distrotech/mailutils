/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2008, 2010-2012 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#include <mailutils/mailutils.h>
#include <mailutils/server.h>

mu_server_t server;

int
echo_conn (int fd, struct sockaddr *s, int len,
	   void *server_data, void *call_data,
	   mu_ip_server_t srv)
{
  pid_t pid;
  char buf[512];
  FILE *in, *out;

  pid = fork ();
  if (pid == -1)
    {
      mu_error ("fork failed: %s", mu_strerror (errno));
      return 0;
    }

  if (pid)
    {
      struct mu_sockaddr *clt_addr;
      int rc = mu_sockaddr_create (&clt_addr, s, len);
      if (rc)
	{
	  mu_error ("mu_sockaddr_create failed: %s", mu_strerror (rc));
	  return 0;
	}
  
      mu_diag_output (MU_DIAG_INFO, "%lu: opened connection %s => %s",
		      (unsigned long) pid,
		      mu_ip_server_addrstr (srv),
		      mu_sockaddr_str (clt_addr));

      mu_sockaddr_free (clt_addr);
      return 0;
    }

  mu_ip_server_shutdown (srv);

  in = fdopen (fd, "r");
  out = fdopen (fd, "w");
  setvbuf (in, NULL, _IOLBF, 0);
  setvbuf (out, NULL, _IOLBF, 0);

  pid = getpid ();
  while (fgets (buf, sizeof (buf), in) > 0)
    {
      int len = strlen (buf);
      if (len > 0 && buf[len-1] == '\n')
	{
	  buf[--len] = 0;
	  if (buf[len-1] == '\r')
	    buf[--len] = 0;
	}
      fprintf (out, "%lu: you said: \"%s\"\r\n", (unsigned long) pid, buf);
    }
  exit (0);
}

int
tcp_conn_handler (int fd, void *conn_data, void *server_data)
{
  mu_ip_server_t tcpsrv = (mu_ip_server_t) conn_data;
  int rc = mu_ip_server_accept (tcpsrv, server_data);
  if (rc && rc != EINTR)
    {
      mu_ip_server_shutdown (tcpsrv);
      return MU_SERVER_CLOSE_CONN;
    }
  return MU_SERVER_SUCCESS;
}

void
tcp_conn_free (void *conn_data, void *server_data)
{
  mu_ip_server_t tcpsrv = (mu_ip_server_t) conn_data;
  mu_ip_server_destroy (&tcpsrv);
}

void
create_server (char *arg)
{
  struct mu_sockaddr *s;
  mu_ip_server_t tcpsrv;
  int rc;
  mu_url_t url, url_hint;
  struct mu_sockaddr_hints hints;

  if (arg[0] == '/')
    url_hint = NULL;
  else
    {
      rc = mu_url_create (&url_hint, "inet://");
      if (rc)
	{
	  mu_error ("cannot create URL hints: %s", mu_strerror (rc));
	  exit (1);
	}
    }
  rc = mu_url_create_hint (&url, arg, MU_URL_PARSE_DEFAULT, url_hint);
  mu_url_destroy (&url_hint);
  if (rc)
    {
      mu_error ("cannot parse URL `%s': %s", arg, mu_strerror (rc));
      exit (1);
    }

  memset (&hints, sizeof(hints), 0);
  hints.flags = MU_AH_PASSIVE;
  hints.socktype = SOCK_STREAM;
  hints.protocol = IPPROTO_TCP;
  rc = mu_sockaddr_from_url (&s, url, &hints); 
  mu_url_destroy (&url);

  if (rc)
    {
      mu_error ("cannot create sockaddr: %s", mu_strerror (rc));
      exit (1);
    }

  MU_ASSERT (mu_ip_server_create (&tcpsrv, s, MU_IP_TCP));
  MU_ASSERT (mu_ip_server_open (tcpsrv));
  MU_ASSERT (mu_ip_server_set_conn (tcpsrv, echo_conn));
  MU_ASSERT (mu_server_add_connection (server,
				       mu_ip_server_get_fd (tcpsrv),
				       tcpsrv,
				       tcp_conn_handler, tcp_conn_free));
}

static int cleanup_needed;

RETSIGTYPE
sig_child (int sig)
{
  cleanup_needed = 1;
  signal (sig, sig_child);
}

int
server_idle (void *server_data)
{
  if (cleanup_needed)
    {
      int status;
      pid_t pid;

      cleanup_needed = 0;
      while ((pid = waitpid (-1, &status, WNOHANG)) > 0)
	{
	  if (WIFEXITED (status))
	    mu_diag_output (MU_DIAG_INFO, "%lu: finished with code %d",
			    (unsigned long) pid,
			    WEXITSTATUS (status));
	  else if (WIFSIGNALED (status))
	    mu_diag_output (MU_DIAG_ERR, "%lu: terminated on signal %d",
			    (unsigned long) pid,
			    WTERMSIG (status));
	  else
	    mu_diag_output (MU_DIAG_ERR, "%lu: terminated (cause unknown)",
			    (unsigned long) pid);
	}
    }
  return 0;
}

int
run ()
{
  int rc;
  signal (SIGCHLD, sig_child);
  rc = mu_server_run (server);
  if (rc)
    mu_error ("%s", mu_strerror (rc));
  mu_server_destroy (&server);
  return rc ? 1 : 0;
}

int
main (int argc, char **argv)
{
  int rc;
  
  mu_set_program_name (argv[0]);
  while ((rc = getopt (argc, argv, "Dd:")) != EOF)
    {
      switch (rc)
	{
	case 'D':
	  mu_debug_line_info = 1;
	  break;
	  
	case 'd':
	  mu_debug_parse_spec (optarg);
	  break;

	default:
	  exit (1);
	}
    }

  argc -= optind;
  argv += optind;

  MU_ASSERT (mu_server_create (&server));
  mu_server_set_idle (server, server_idle);
  while (argc--)
    create_server (*argv++);
  return run ();
}


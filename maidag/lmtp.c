/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "maidag.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <mu_umaxtostr.h>

mu_list_t lmtp_groups;

static mu_stream_t
lmtp_transcript (mu_stream_t iostream)
{
  int rc;
  mu_stream_t dstr, xstr;
      
  rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
  if (rc)
    mu_error (_("cannot create debug stream; transcript disabled: %s"),
	      mu_strerror (rc));
  else
    {
      rc = mu_xscript_stream_create (&xstr, iostream, dstr, NULL);
      if (rc)
	mu_error (_("cannot create transcript stream: %s"),
		  mu_strerror (rc));
      else
	{
	  /* FIXME: Would do a mu_stream_unref (iostream) here,
	     however mu_xscript_stream_create *may* steal the reference.
	     This should be fixed in mu_xscript_stream_create. */
	  iostream = xstr;
	}
    }
  return iostream;
}

void
lmtp_reply (mu_stream_t iostr, char *code, char *enh, char *fmt, ...)
{
  va_list ap;
  char *str = NULL;
  size_t size = 0;
  
  va_start (ap, fmt);
  mu_vasnprintf (&str, &size, fmt, ap);
  va_end (ap);

  if (!str)
    {
      mu_error (_("not enough memory"));
      exit (EX_TEMPFAIL);
    }

  while (*str)
    {
      char *end = strchr (str, '\n');

      if (end)
	{
	  size_t len = end - str;
	  mu_stream_printf (iostr, "%s-", code);
	  if (enh)
	    mu_stream_printf (iostr, "%s ", enh);
	  mu_stream_printf (iostr, "%.*s\r\n", (int) len, str);
	  for (str = end; *str && *str == '\n'; str++);
	}
      else
	{
	  mu_stream_printf (iostr, "%s ", code);
	  if (enh)
	    mu_stream_printf (iostr, "%s ", enh);
	  mu_stream_printf (iostr, "%s\r\n", str);
	  str += strlen (str);
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
mu_message_t mesg;     /* Collected message */


int
cfun_unknown (mu_stream_t iostr, char *arg)
{
  lmtp_reply (iostr, "500", "5.5.1", "Command unrecognized");
  return 0;
}


static void
add_default_domain (char *str, int len, char **pret)
{
  *pret = malloc (len + 1 + strlen (lhlo_domain) + 1);
  if (!*pret)
    {
      mu_error (_("not enough memory"));
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
cfun_mail_from (mu_stream_t iostr, char *arg)
{
  if (*arg == 0)
    {
      lmtp_reply (iostr, "501", "5.5.2", "Syntax error");
      return 1;
    }

  if (check_address (arg, 1, &mail_from))
    {
      lmtp_reply (iostr, "553", "5.1.8", "Address format error");
      return 1;
    }
  lmtp_reply (iostr, "250", "2.1.0", "Go ahead");
  return 0;
}


int
cfun_rcpt_to (mu_stream_t iostr, char *arg)
{
  char *user;
  struct mu_auth_data *auth;
  
  if (*arg == 0)
    {
      lmtp_reply (iostr, "501", "5.5.2", "Syntax error");
      return 1;
    }

  /* FIXME: Check if domain is OK */
  if (check_address (arg, 0, &user))
    {
      lmtp_reply (iostr, "553", "5.1.8", "Address format error");
      return 1;
    }
  auth = mu_get_auth_by_name (user);
  if (!auth)
    {
      lmtp_reply (iostr, "550", "5.1.1", "User unknown");
      free (user);
      return 1;
    }
  mu_auth_data_free (auth);
  if (!rcpt_list)
    {
      mu_list_create (&rcpt_list);
      mu_list_set_destroy_item (rcpt_list, mu_list_free_item);
    }
  mu_list_append (rcpt_list, user);
  lmtp_reply (iostr, "250", "2.1.5", "Go ahead");
  return 0;
}  


int
dot_temp_fail (void *item, void *cbdata)
{
  char *name = item;
  mu_stream_t iostr = cbdata;
  lmtp_reply (iostr, "450", "4.1.0", "%s: temporary failure", name);
  return 0;
}

int
dot_deliver (void *item, void *cbdata)
{
  char *name = item;
  mu_stream_t iostr = cbdata;
  char *errp = NULL;
  
  switch (deliver_to_user (mesg, name, &errp))
    {
    case 0:
      lmtp_reply (iostr, "250", "2.0.0", "%s: delivered", name);
      break;

    case EX_UNAVAILABLE:
      if (errp)
	lmtp_reply (iostr, "553", "5.1.8", "%s", errp);
      else
	lmtp_reply (iostr, "553", "5.1.8", "%s: delivery failed", name);
      break;

    default:
      if (errp)
	lmtp_reply (iostr, "450", "4.1.0", "%s", errp);
      else
	lmtp_reply (iostr, "450", "4.1.0",
		    "%s: temporary failure, try again later",
		    name);
      break;
    }
  free (errp);
  return 0;
}

int
cfun_data (mu_stream_t iostr, char *arg)
{
  int rc;
  mu_stream_t flt, tempstr;
  time_t t;
  struct tm *tm;
  int xlev = MU_XSCRIPT_PAYLOAD, xlev_switch = 0;
  
  if (*arg)
    {
      lmtp_reply (iostr, "501", "5.5.2", "Syntax error");
      return 1;
    }

  rc = mu_filter_create (&flt, iostr, "CRLFDOT", MU_FILTER_DECODE,
			 MU_STREAM_READ|MU_STREAM_WRTHRU);
  if (rc)
    {
      maidag_error (_("unable to open filter: %s"),
		    mu_strerror (rc));
      lmtp_reply (iostr, "450", "4.1.0", "Temporary failure, try again later");
      return 1;
    }

  rc = mu_temp_file_stream_create (&tempstr, NULL, 0);
  if (rc)
    {
      maidag_error (_("unable to open temporary file: %s"), mu_strerror (rc));
      mu_stream_destroy (&flt);
      return 1;
    }

  /* Write out envelope */
  time (&t);
  tm = gmtime (&t);
  rc = mu_stream_printf (tempstr, "From %s ", mail_from);
  if (rc == 0)
    rc = mu_c_streamftime (tempstr, "%c%n", tm, NULL);
  if (rc)
    {
      maidag_error (_("copy error: %s"), mu_strerror (rc));
      mu_stream_destroy (&flt);
      mu_stream_destroy (&tempstr);
      mu_list_foreach (rcpt_list, dot_temp_fail, iostr);
    }

  lmtp_reply (iostr, "354", NULL, "Go ahead");

  if (mu_stream_ioctl (iostr, MU_IOCTL_XSCRIPTSTREAM,
                       MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
    xlev_switch = 1;
  rc = mu_stream_copy (tempstr, flt, 0, NULL);
  mu_stream_destroy (&flt);
  if (xlev_switch)
    mu_stream_ioctl (iostr, MU_IOCTL_XSCRIPTSTREAM,
                     MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev);
  if (rc)
    {
      maidag_error (_("copy error: %s"), mu_strerror (rc));
      mu_list_foreach (rcpt_list, dot_temp_fail, iostr);
    }

  rc = mu_stream_to_message (tempstr, &mesg);
  mu_stream_unref (tempstr);
  if (rc)
    {
      maidag_error (_("error creating temporary message: %s"),
		    mu_strerror (rc));
      mu_list_foreach (rcpt_list, dot_temp_fail, iostr);
    }
  
  rc = mu_list_foreach (rcpt_list, dot_deliver, iostr);

  mu_message_destroy (&mesg, mu_message_get_owner (mesg));
  if (rc)
    mu_list_foreach (rcpt_list, dot_temp_fail, iostr);

  return 0;
}


int
cfun_rset (mu_stream_t iostr, char *arg)
{
  free (lhlo_domain);
  free (mail_from);
  mu_list_destroy (&rcpt_list);
  mu_message_destroy (&mesg, mu_message_get_owner (mesg));
  lmtp_reply (iostr, "250", "2.0.0", "OK, forgotten");
  return 0;
}


char *capa_str = "ENHANCEDSTATUSCODES\n\
PIPELINING\n\
8BITMIME\n\
HELP";

int
cfun_lhlo (mu_stream_t iostr, char *arg)
{
  if (*arg == 0)
    {
      lmtp_reply (iostr, "501", "5.0.0", "Syntax error");
      return 1;
    }
  lhlo_domain = strdup (arg);
  if (!lhlo_domain)
    {
      lmtp_reply (iostr, "410", "4.0.0",
                  "Local error; please try again later");
      return 1;
    }
  lmtp_reply (iostr, "250", NULL, "Hello\n");
  lmtp_reply (iostr, "250", NULL, capa_str);
  return 0;
}


int
cfun_quit (mu_stream_t iostr, char *arg)
{
  lmtp_reply (iostr, "221", "2.0.0", "Bye");
  return 0;
}


int
cfun_help (mu_stream_t iostr, char *arg)
{
  lmtp_reply (iostr, "200", "2.0.0", "Man, help yourself");
  return 0;
}


struct command_tab
{
  char *cmd_verb;
  int cmd_len;
  enum lmtp_command cmd_code;
  int (*cmd_fun) (mu_stream_t, char *);
} command_tab[] = {
#define S(s) #s, (sizeof #s - 1)
  { S(lhlo), cmd_lhlo, cfun_lhlo },
  { S(mail from:), cmd_mail, cfun_mail_from },   
  { S(rcpt to:), cmd_rcpt, cfun_rcpt_to },   
  { S(data), cmd_data, cfun_data },
  { S(quit), cmd_quit, cfun_quit },
  { S(rset), cmd_rset, cfun_rset },
  { S(help), cmd_help, cfun_help },
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
	  && mu_c_strncasecmp (cp->cmd_verb, buf, cp->cmd_len) == 0)
	{
	  *sp = buf + cp->cmd_len;
	  return cp;
	}
    }
  return cp;
}

static int
to_fgets (mu_stream_t iostr, char **pbuf, size_t *psize, size_t *pnread,
	  unsigned int timeout)
{
  int rc;
  
  alarm (timeout);
  rc = mu_stream_getline (iostr, pbuf, psize, pnread);
  alarm (0);
  return rc;
}

int
lmtp_loop (mu_stream_t iostr, unsigned int timeout)
{
  size_t size = 0, n;
  char *buf = NULL;
  enum lmtp_state state = state_init;

  lmtp_reply (iostr, "220", NULL, "At your service");
  while (to_fgets (iostr, &buf, &size, &n, timeout) == 0 && n)
    {
      char *sp;
      struct command_tab *cp = getcmd (buf, &sp);
      enum lmtp_command cmd = cp->cmd_code;
      enum lmtp_state next_state = transtab[cmd][state];

      mu_rtrim_class (sp, MU_CTYPE_ENDLN);

      if (next_state != state_none)
	{
	  if (cp->cmd_fun)
	    {
	      sp = mu_str_skip_class (sp, MU_CTYPE_SPACE);
	      if (cp->cmd_fun (iostr, sp))
		continue;
	    }
	  state = next_state;
	}
      else
	lmtp_reply (iostr, "503", "5.0.0", "Syntax error");
      
      if (state == state_end)
	break;
    }
  return 0;
}

typedef union
{
  struct sockaddr sa;
  struct sockaddr_in s_in;
  struct sockaddr_un s_un;
} all_addr_t;

int
lmtp_connection (int fd, struct sockaddr *sa, int salen,
		 struct mu_srv_config *pconf,
		 void *data)
{
  mu_stream_t str;
  int rc;
  
  rc = mu_fd_stream_create (&str, NULL, fd, MU_STREAM_RDWR);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_fd_stream_create", NULL, rc);
      return rc;
    }
  mu_stream_set_buffer (str, mu_buffer_line, 0);

  if (pconf->transcript || maidag_transcript)
    str = lmtp_transcript (str);
  lmtp_loop (str, pconf->timeout);
  mu_stream_destroy (&str);
  return 0;
}

static int
lmtp_set_privs ()
{
  gid_t gid;
  
  if (lmtp_groups)
    {
      gid_t *gidset = NULL;
      size_t size = 0;
      size_t j = 0;
      mu_iterator_t itr;
      int rc;

      rc = mu_list_count (lmtp_groups, &size);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_list_count", NULL, rc);
	  return EX_UNAVAILABLE;
	}
      if (size == 0)
	return 0; /* nothing to do */
      gidset = calloc (size, sizeof (gidset[0]));
      if (!gidset)
	{
	  mu_error (_("not enough memory"));
	  return EX_UNAVAILABLE;
	}
      if (mu_list_get_iterator (lmtp_groups, &itr) == 0)
	{
	  for (mu_iterator_first (itr);
	       !mu_iterator_is_done (itr); mu_iterator_next (itr)) 
	    mu_iterator_current (itr,
				 (void **)(gidset + j++));
	  mu_iterator_destroy (&itr);
	}
      gid = gidset[0];
      rc = setgroups (j, gidset);
      free (gidset);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "setgroups", NULL, errno);
	  return EX_UNAVAILABLE;
	}
    }
  else
    {
      struct group *gr = getgrnam ("mail");
      if (gr == NULL)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "getgrnam", "mail", errno);
	  return EX_UNAVAILABLE;
	}
      gid = gr->gr_gid;
    }
  if (setgid (gid) == -1)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "setgid", "mail", errno);
      return EX_UNAVAILABLE;
    }
  return 0;
}
		
int
maidag_lmtp_server ()
{
  int rc = lmtp_set_privs ();
  if (rc)
    return rc;

  if (mu_m_server_mode (server) == MODE_DAEMON)
    {
      int status;
      mu_m_server_begin (server);
      status = mu_m_server_run (server);
      mu_m_server_end (server);
      mu_m_server_destroy (&server);
      if (status)
	return EX_CONFIG;
    }
  else
    {
      mu_stream_t str, istream, ostream;

      rc = mu_stdio_stream_create (&istream, MU_STDIN_FD, MU_STREAM_READ);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create",
			   "MU_STDIN_FD", rc);
	  return EX_UNAVAILABLE;
	} 
 
      rc = mu_stdio_stream_create (&ostream, MU_STDOUT_FD, MU_STREAM_WRITE);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create",
			   "MU_STDOUT_FD", rc);
	  return 1;
	} 

      rc = mu_iostream_create (&str, istream, ostream);
      mu_stream_unref (istream);
      mu_stream_unref (ostream);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_iostream_create", NULL, rc);
	  return 1;
	} 

      if (maidag_transcript)
	str = lmtp_transcript (str);

      rc = lmtp_loop (str, 0);
      mu_stream_destroy (&str);
      return rc;
    }
}

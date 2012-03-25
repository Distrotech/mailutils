/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

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
#include <config.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/nls.h>
#include <mailutils/acl.h>
#include <mailutils/wordsplit.h>
#include <mailutils/list.h>
#include <mailutils/debug.h>
#include <mailutils/sys/debcat.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/kwd.h>
#include <mailutils/io.h>
#include <mailutils/util.h>
#include <mailutils/sockaddr.h>
#include <mailutils/cidr.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

#ifndef MU_INADDR_BYTES
#define MU_INADDR_BYTES 16
#endif

struct _mu_acl_entry
{
  mu_acl_action_t action;
  void *arg;
  struct mu_cidr cidr;
};

struct _mu_acl
{
  mu_list_t aclist;
  char **envv;
  size_t envc;
  size_t envn;
};

struct run_closure
{
  unsigned idx;
  struct mu_cidr addr;

  char **env;
  char ipstr[40];
  char *addrstr;
  char *numbuf;
  mu_acl_result_t *result;
};


static void
_destroy_acl_entry (void *item)
{
  struct _mu_acl_entry *p = item;
  free (p);
  /* FIXME: free arg? */
}

int
mu_acl_entry_create (struct _mu_acl_entry **pent,
		     mu_acl_action_t action, void *data,
		     struct mu_cidr *cidr)
{
  struct _mu_acl_entry *p = malloc (sizeof (*p));
  if (!p)
    return EINVAL;

  p->action = action;
  p->arg = data;
  memcpy (&p->cidr, cidr, sizeof (p->cidr));

  *pent = p;
  return 0;
}


int
mu_acl_create (mu_acl_t *pacl)
{
  int rc;
  mu_acl_t acl;

  acl = calloc (1, sizeof (*acl));
  if (!acl)
    return errno;
  rc = mu_list_create (&acl->aclist);
  if (rc)
    free (acl);
  else
    *pacl = acl;
  mu_list_set_destroy_item (acl->aclist, _destroy_acl_entry);

  return rc;
}

int
mu_acl_count (mu_acl_t acl, size_t *pcount)
{
  if (!acl)
    return EINVAL;
  return mu_list_count (acl->aclist, pcount);
}

int
mu_acl_destroy (mu_acl_t *pacl)
{
  size_t i;
  
  mu_acl_t acl;
  if (!pacl || !*pacl)
    return EINVAL;
  acl = *pacl;
  mu_list_destroy (&acl->aclist);
  for (i = 0; i < acl->envc && acl->envv[i]; i++)
    free (acl->envv[i]);
  free (acl->envv);
  free (acl);
  *pacl = acl;
  return 0;
}
		   
int
mu_acl_get_iterator (mu_acl_t acl, mu_iterator_t *pitr)
{
  if (!acl)
    return EINVAL;
  return mu_list_get_iterator (acl->aclist, pitr);
}

int
mu_acl_append (mu_acl_t acl, mu_acl_action_t act,
	       void *data, struct mu_cidr *cidr)
{
  int rc;
  struct _mu_acl_entry *ent;
  
  if (!acl)
    return EINVAL;
  rc = mu_acl_entry_create (&ent, act, data, cidr);
  if (rc)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("Cannot allocate ACL entry: %s", mu_strerror (rc)));
      return ENOMEM;
    }
  
  rc = mu_list_append (acl->aclist, ent);
  if (rc)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("Cannot append ACL entry: %s", mu_strerror (rc)));
      free (ent);
    }
  return rc;
}

int
mu_acl_prepend (mu_acl_t acl, mu_acl_action_t act, void *data,
		struct mu_cidr *cidr)
{
  int rc;
  struct _mu_acl_entry *ent;
  
  if (!acl)
    return EINVAL;
  rc = mu_acl_entry_create (&ent, act, data, cidr);
  if (rc)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR,
                ("Cannot allocate ACL entry: %s", mu_strerror (rc)));
      return ENOMEM;
    }
  rc = mu_list_prepend (acl->aclist, ent); 
  if (rc)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("Cannot prepend ACL entry: %s", mu_strerror (rc)));
      free (ent);
    }
  return rc;
}

int
mu_acl_insert (mu_acl_t acl, size_t pos, int before,
	       mu_acl_action_t act, void *data, struct mu_cidr *cidr)
{
  int rc;
  void *ptr;
  struct _mu_acl_entry *ent;
  
  if (!acl)
    return EINVAL;
  
  rc = mu_list_get (acl->aclist, pos, &ptr);
  if (rc)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR,
                ("No such entry %lu", (unsigned long) pos));
      return rc;
    }
  rc = mu_acl_entry_create (&ent, act, data, cidr);
  if (!ent)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("Cannot allocate ACL entry: %s", mu_strerror (rc)));
      return ENOMEM;
    }
  rc = mu_list_insert (acl->aclist, ptr, ent, before);
  if (rc)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("Cannot insert ACL entry: %s", mu_strerror (rc)));
      free (ent);
    }
  return rc;
}


static mu_kwd_t action_tab[] = {
  { "accept", mu_acl_accept },
  { "deny", mu_acl_deny },
  { "log", mu_acl_log },
  { "exec", mu_acl_exec },
  { "ifexec", mu_acl_ifexec },
  { NULL }
};

int
mu_acl_action_to_string (mu_acl_action_t act, const char **pstr)
{
  return mu_kwd_xlat_tok (action_tab, act, pstr);
}

int
mu_acl_string_to_action (const char *str, mu_acl_action_t *pres)
{
  int x;
  int rc = mu_kwd_xlat_name (action_tab, str, &x);
  if (rc == 0)
    *pres = x;
  return rc;
}

int
_acl_match (struct _mu_acl_entry *ent, struct run_closure *rp)
{
#define RESMATCH(word)                                   \
  if (mu_debug_level_p (MU_DEBCAT_ACL, MU_DEBUG_TRACE9))     \
    mu_debug_log_end ("%s; ", word);
							      
  if (mu_debug_level_p (MU_DEBCAT_ACL, MU_DEBUG_TRACE9))
    {
      char *s = NULL;
      int rc;
      
      if (ent->cidr.len && (rc = mu_cidr_format (&ent->cidr, 0, &s)))
        {
          mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR,
                    ("mu_cidr_format: %s", mu_strerror (rc)));
          return 1;
        }
      if (!rp->addrstr)
        mu_cidr_format (&rp->addr, MU_CIDR_FMT_ADDRONLY, &rp->addrstr);
      mu_debug_log_begin ("Does %s match %s? ", s ? s : "any", rp->addrstr);
      free (s);
    }

  if (ent->cidr.len > 0 && mu_cidr_match (&ent->cidr, &rp->addr))
    {
      RESMATCH ("no");
      return 1;
    }
  RESMATCH ("yes");
  return 0;
}

#define SEQ(s, n, l) \
  (((l) == (sizeof(s) - 1)) && memcmp (s, n, l) == 0)

static const char *
acl_getvar (const char *name, size_t nlen, void *data)
{
  struct run_closure *rp = data;

  if (SEQ ("family", name, nlen))
    {
      switch (rp->addr.family)
	{
	case AF_INET:
	  return "AF_INET";

#ifdef MAILUTILS_IPV6
	case AF_INET6:
	  return "AF_INET6";
#endif
      
	case AF_UNIX:
	  return "AF_UNIX";

	default:
	  return NULL;
	}
    }
  
  if (SEQ ("aclno", name, nlen))
    {
      if (!rp->numbuf && mu_asprintf (&rp->numbuf, "%u", rp->idx))
	return NULL;
      return rp->numbuf;
    }
  
  if (SEQ ("address", name, nlen))
    {
      if (!rp->addrstr)
	mu_cidr_format (&rp->addr, MU_CIDR_FMT_ADDRONLY, &rp->addrstr);
      return rp->addrstr;
    }

#if 0
  /* FIXME?: */
  if (SEQ ("port", name, nlen))
    {
      if (!rp->portbuf &&
	  mu_asprintf (&rp->portbuf, "%hu", ntohs (s_in->sin_port)))
	return NULL;
      return rp->portbuf;
    }
#endif

  return NULL;
}

static int
expand_arg (const char *cmdline, struct run_closure *rp, char **s)
{
  int rc;
  struct mu_wordsplit ws;
  int envflag = 0;
  
  mu_debug (MU_DEBCAT_ACL, MU_DEBUG_TRACE, ("Expanding \"%s\"", cmdline));
  if (rp->env)
    {
      ws.ws_env = (const char **) rp->env;
      envflag = MU_WRDSF_ENV;
    }
  ws.ws_getvar = acl_getvar;
  ws.ws_closure = rp;
  rc = mu_wordsplit (cmdline, &ws,
		     MU_WRDSF_NOSPLIT | MU_WRDSF_NOCMD |
		     envflag | MU_WRDSF_ENV_KV |
		     MU_WRDSF_GETVAR | MU_WRDSF_CLOSURE);

  if (rc == 0)
    {
      *s = strdup (ws.ws_wordv[0]);
      mu_wordsplit_free (&ws);
      if (!*s)
	{
	  mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
	            ("failed: not enough memory."));
	  return ENOMEM;
	}
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_TRACE, ("Expansion: \"%s\". ", *s));
    }
  else
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("failed: %s", mu_wordsplit_strerror (&ws)));
      rc = errno;
    }
  return rc;
}

static int
spawn_prog (const char *cmdline, int *pstatus, struct run_closure *rp)
{
  char *s;
  pid_t pid;

  if (expand_arg (cmdline, rp, &s))
    {
      s = strdup (cmdline);
      if (!s)
        return ENOMEM;
    }
  pid = fork ();
  if (pid == 0)
    {
      int i;
      struct mu_wordsplit ws;

      if (mu_wordsplit (s, &ws, MU_WRDSF_DEFFLAGS))
	{
	  mu_error (_("cannot split line `%s': %s"), s,
		    mu_wordsplit_strerror (&ws));
	  _exit (127);
	}
      
      for (i = mu_getmaxfd (); i > 2; i--)
	close (i);
      execvp (ws.ws_wordv[0], ws.ws_wordv);
      _exit (127);
    }

  free (s);

  if (pid == (pid_t)-1)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
                ("cannot fork: %s", mu_strerror (errno)));
      return errno;
    }

  if (pstatus)
    {
      int status;
      waitpid (pid, &status, 0);
      if (WIFEXITED (status))
	{
	  status = WEXITSTATUS (status);
	  mu_debug (MU_DEBCAT_ACL, MU_DEBUG_TRACE,
		    ("Program finished with code %d.", status));
	  *pstatus = status;
	}
      else if (WIFSIGNALED (status))
	{
          mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR,
		    ("Program terminated on signal %d.",
		     WTERMSIG (status)));
	  return MU_ERR_PROCESS_SIGNALED;
	}
      else
	return MU_ERR_PROCESS_UNKNOWN_FAILURE;
    }
	
  return 0;
}
	    

int
_run_entry (void *item, void *data)
{
  int status = 0;
  struct _mu_acl_entry *ent = item;
  struct run_closure *rp = data;

  rp->idx++;

  if (mu_debug_level_p (MU_DEBCAT_ACL, MU_DEBUG_TRACE9))
    {
      const char *s = "undefined";
      mu_acl_action_to_string (ent->action, &s);
      mu_debug_log_begin ("%d:%s: ", rp->idx, s);
    }
  
  if (_acl_match (ent, rp) == 0)
    {
      switch (ent->action)
	{
	case mu_acl_accept:
	  *rp->result = mu_acl_result_accept;
	  status = 1;
	  break;
      
	case mu_acl_deny:
	  *rp->result = mu_acl_result_deny;
	  status = 1;
	  break;
      
	case mu_acl_log:
	  {
	    char *s;
	    if (ent->arg && expand_arg (ent->arg, rp, &s) == 0)
	      {
		mu_diag_output (MU_DIAG_INFO, "%s", s);
		free (s);
	      }
	    else
	      {
		if (!rp->addrstr)
		  mu_cidr_format (&rp->addr, MU_CIDR_FMT_ADDRONLY,
				  &rp->addrstr);
		mu_diag_output (MU_DIAG_INFO, "%s", rp->addrstr);
	      }
	  }
	  break;
	  
	case mu_acl_exec:
	  spawn_prog (ent->arg, NULL, rp);
	  break;
	  
	case mu_acl_ifexec:
	  {
	    int prog_status;
	    int rc = spawn_prog (ent->arg, &prog_status, rp);
	    if (rc == 0)
	      {
		switch (prog_status)
		  {
		  case 0:
		    *rp->result = mu_acl_result_accept;
		    status = 1;
		    break;
		    
		  case 1:
		    *rp->result = mu_acl_result_deny;
		    status = 1;
		  }
	      }
	  }
	  break;
	}
    }
  
  if (mu_debug_level_p (MU_DEBCAT_ACL, MU_DEBUG_TRACE9))
    mu_stream_flush (mu_strerr);
  
  return status;
}

int
mu_acl_check_sockaddr (mu_acl_t acl, const struct sockaddr *sa, int salen,
		       mu_acl_result_t *pres)
{
  int rc;
  struct run_closure r;
  
  if (!acl)
    return EINVAL;

  memset (&r, 0, sizeof (r));
  if (sa->sa_family == AF_UNIX)
    {
      *pres = mu_acl_result_accept;
      return 0;
    }
  rc = mu_cidr_from_sockaddr (&r.addr, sa);
  if (rc)
    return rc;
  
  if (mu_debug_level_p (MU_DEBCAT_ACL, MU_DEBUG_TRACE9))
    {
      mu_cidr_format (&r.addr, MU_CIDR_FMT_ADDRONLY, &r.addrstr);
      mu_debug_log_begin ("Checking sockaddr %s", r.addrstr);
      mu_debug_log_nl ();
    }

  r.idx = 0;
  r.result = pres;
  r.env = acl->envv;
  *r.result = mu_acl_result_undefined;
  r.numbuf = NULL;
  mu_list_foreach (acl->aclist, _run_entry, &r);
  free (r.numbuf);
  free (r.addrstr);
  return 0;
}

int
mu_acl_check_inaddr (mu_acl_t acl, const struct in_addr *inp,
		     mu_acl_result_t *pres)
{
  struct sockaddr_in cs;
  int len = sizeof cs;

  cs.sin_family = AF_INET;
  cs.sin_addr = *inp;
  cs.sin_addr.s_addr = ntohl (cs.sin_addr.s_addr);
  return mu_acl_check_sockaddr (acl, (struct sockaddr *) &cs, len, pres);
}
  
int
mu_acl_check_ipv4 (mu_acl_t acl, unsigned int addr, mu_acl_result_t *pres)
{
  struct in_addr in;

  in.s_addr = addr;
  return mu_acl_check_inaddr (acl, &in, pres);
}

int
mu_acl_check_fd (mu_acl_t acl, int fd, mu_acl_result_t *pres)
{
  union
  {
    struct sockaddr sa;
    struct sockaddr_in in;
#ifdef MAILUTILS_IPV6
    struct sockaddr_in6 in6;
#endif
  } addr;
  socklen_t len = sizeof addr;

  if (getpeername (fd, &addr.sa, &len) < 0)
    {
      mu_debug (MU_DEBCAT_ACL, MU_DEBUG_ERROR, 
		("Cannot obtain IP address of client: %s",
		 mu_strerror (errno)));
      return MU_ERR_FAILURE;
    }

  return mu_acl_check_sockaddr (acl, &addr.sa, len, pres);
}

static int
_acl_getenv (mu_acl_t acl, const char *name, size_t *pres)
{
  size_t i;

  if (!acl->envv)
    return MU_ERR_NOENT;
  for (i = 0; i < acl->envc; i++)
    if (strcmp (acl->envv[i], name) == 0)
      {
	*pres = i;
	return 0;
      }
  return MU_ERR_NOENT;
}

const char *
mu_acl_getenv (mu_acl_t acl, const char *name)
{
  size_t i;
  
  if (_acl_getenv (acl, name, &i) == 0)
    {
      return acl->envv[i + 1];
    }
  return NULL;
}

static int
_acl_env_store (mu_acl_t acl, int i, const char *val)
{
  char *copy = strdup (val);
  if (!copy)
    return ENOMEM;
  free (acl->envv[i]);
  acl->envv[i] = copy;
  return 0;
}

int
mu_acl_setenv (mu_acl_t acl, const char *name, const char *val)
{
  size_t i;

  if (_acl_getenv (acl, name, &i) == 0)
    {
      if (!val)
	{
	  free (acl->envv[i]);
	  free (acl->envv[i + 1]);
	  memmove (acl->envv + i, acl->envv + i + 3,
		   (acl->envn + 1 - (i + 3)) * sizeof (acl->envv[0]));
	  acl->envn -= 2;
	  return 0;
	}
      return _acl_env_store (acl, i + 1, val);
    }

  if (!acl->envv || acl->envn + 1 == acl->envc)
    {
      char **p;

      if (!val)
	return 0;
      
      if (acl->envv == NULL)
	p = calloc (3, sizeof (acl->envv[0]));
      else
	{
	  p = realloc (acl->envv, (acl->envc + 3) * sizeof (acl->envv[0]));
	  if (!p)
	    return ENOMEM;
	  p[acl->envc] = NULL;
	}
      
      acl->envv = p;
      acl->envc += 3;
    }

  if (_acl_env_store (acl, acl->envn, name))
    return ENOMEM;
  if (_acl_env_store (acl, acl->envn + 1, val))
    {
      free (acl->envv[acl->envn]);
      acl->envv[acl->envn] = NULL;
      return ENOMEM;
    }
  acl->envn += 2;

  return 0;
}

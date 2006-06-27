/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2005, 2006 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <mailutils/address.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/iterator.h>
#include <mailutils/mutil.h>
#include <mailutils/parse822.h>
#include <mailutils/mu_auth.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/envelope.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>

#include <registrar0.h>

/* convert a sequence of hex characters into an integer */

unsigned long
mu_hex2ul (char hex)
{
  if (hex >= '0' && hex <= '9')
   return hex - '0';

  if (hex >= 'a' && hex <= 'z')
    return hex - 'a';

  if (hex >= 'A' && hex <= 'Z')
    return hex - 'A';

  return -1;
}

size_t
mu_hexstr2ul (unsigned long *ul, const char *hex, size_t len)
{
  size_t r;

  *ul = 0;

  for (r = 0; r < len; r++)
    {
      unsigned long v = mu_hex2ul (hex[r]);

      if (v == (unsigned long)-1)
	return r;

      *ul = *ul * 16 + v;
    }
  return r;
}


char *
mu_get_homedir (void)
{
  char *homedir = getenv ("HOME");
  if (homedir)
    homedir = strdup (homedir);
  else
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (geteuid ());
      if (!auth)
	return NULL;
      homedir = strdup (auth->dir);
      mu_auth_data_free (auth);
    }
  return homedir;
}

char *
mu_getcwd ()
{
  char *ret;
  unsigned path_max;
  char buf[128];

  errno = 0;
  ret = getcwd (buf, sizeof (buf));
  if (ret != NULL)
    return strdup (buf);

  if (errno != ERANGE)
    return NULL;

  path_max = 128;
  path_max += 2;                /* The getcwd docs say to do this. */

  for (;;)
    {
      char *cwd = (char *) malloc (path_max);

      errno = 0;
      ret = getcwd (cwd, path_max);
      if (ret != NULL)
        return ret;
      if (errno != ERANGE)
        {
          int save_errno = errno;
          free (cwd);
          errno = save_errno;
          return NULL;
        }

      free (cwd);

      path_max += path_max / 16;
      path_max += 32;
    }
  /* oops?  */
  return NULL;
}

char *
mu_get_full_path (const char *file)
{
  char *p = NULL;

  if (!file)
    p = mu_getcwd ();
  else if (*file != '/')
    {
      char *cwd = mu_getcwd ();
      if (cwd)
	{
	  p = calloc (strlen (cwd) + 1 + strlen (file) + 1, 1);
	  if (p)
	    sprintf (p, "%s/%s", cwd, file);
	  free (cwd);
	}
    }

  if (!p)
    p = strdup (file);
  return p;
}

/* NOTE: Allocates Memory.  */
/* Expand: ~ --> /home/user and to ~guest --> /home/guest.  */
char *
mu_tilde_expansion (const char *ref, const char *delim, const char *homedir)
{
  char *base = strdup (ref);
  char *home = NULL;
  char *proto = NULL;
  size_t proto_len = 0;
  char *p;

  for (p = base; *p && isascii (*p) && isalnum (*p); p++)
    ;
  
  if (*p == ':')
    {
      p++;
      proto_len = p - base;
      proto = malloc (proto_len + 1);
      if (!proto)
	return NULL;
      memcpy (proto, base, proto_len);
      proto[proto_len] = 0;
      /* Allow for extra pair of slashes after the protocol specifier */
      if (*p == delim[0])
	p++;
      if (*p == delim[0])
	p++;
    }
  else
    p = base;
  
  if (*p == '~')
    {
      p++;
      if (*p == delim[0] || *p == '\0')
        {
	  char *s;
	  if (!homedir)
	    {
	      home = mu_get_homedir ();
	      if (!home)
		return base;
	      homedir = home;
	    }
	  s = calloc (proto_len + strlen (homedir) + strlen (p) + 1, 1);
	  if (proto_len)
	    strcpy (s, proto);
	  else
	    s[0] = 0;
          strcat (s, homedir);
          strcat (s, p);
          free (base);
          base = s;
        }
      else
        {
          struct mu_auth_data *auth;
          char *s = p;
          char *name;
          while (*s && *s != delim[0])
            s++;
          name = calloc (s - p + 1, 1);
          memcpy (name, p, s - p);
          name[s - p] = '\0';
	  
          auth = mu_get_auth_by_name (name);
          free (name);
          if (auth)
            {
              char *buf = calloc (proto_len + strlen (auth->dir)
				  + strlen (s) + 1, 1);
	      if (proto_len)
		strcpy (buf, proto);
	      else
		buf[0] = 0;
	      strcat (buf, auth->dir);
              strcat (buf, s);
              free (base);
              base = buf;
	      mu_auth_data_free (auth);
            }
        }
    }
  if (home)
    free (home);
  return base;
}

/* Smart strncpy that always add the null and returns the number of bytes
   written.  */
size_t
mu_cpystr (char *dst, const char *src, size_t size)
{
  size_t len = src ? strlen (src) : 0 ;
  if (dst == NULL || size == 0)
    return len;
  if (len >= size)
    len = size - 1;
  memcpy (dst, src, len);
  dst[len] = '\0';
  return len;
}

/* General retrieve stack support: */

void
mu_register_retriever (mu_list_t *pflist, mu_retrieve_fp fun)
{
  if (!*pflist && mu_list_create (pflist))
    return;
  mu_list_append (*pflist, fun);
}

void *
mu_retrieve (mu_list_t flist, void *data)
{
  void *p = NULL;
  mu_iterator_t itr;

  if (mu_list_get_iterator (flist, &itr) == 0)
    {
      mu_retrieve_fp fun;
      for (mu_iterator_first (itr); !p && !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  mu_iterator_current (itr, (void **)&fun);
	  p = (*fun) (data);
	}

      mu_iterator_destroy (&itr);
    }
  return p;
}

int
mu_get_host_name (char **host)
{
  char hostname[MAXHOSTNAMELEN + 1];
  struct hostent *hp = NULL;
  char *domain = NULL;

  gethostname (hostname, sizeof hostname);
  hostname[sizeof (hostname) - 1] = 0;

  if ((hp = gethostbyname (hostname)))
    domain = hp->h_name;
  else
    domain = hostname;

  domain = strdup (domain);

  if (!domain)
    return ENOMEM;

  *host = domain;

  return 0;
}

/*
 * Functions used to convert unix mailbox/user names into RFC822 addr-specs.
 */

static char *mu_user_email = 0;

int
mu_set_user_email (const char *candidate)
{
  int err = 0;
  mu_address_t addr = NULL;
  size_t emailno = 0;
  char *email = NULL;
  char *domain = NULL;
  
  if ((err = mu_address_create (&addr, candidate)) != 0)
    return err;

  if ((err = mu_address_get_email_count (addr, &emailno)) != 0)
    goto cleanup;

  if (emailno != 1)
    {
      errno = EINVAL;
      goto cleanup;
    }

  if ((err = mu_address_aget_email (addr, 1, &email)) != 0)
    goto cleanup;

  if (mu_user_email)
    free (mu_user_email);

  mu_user_email = email;

  mu_address_aget_domain (addr, 1, &domain);
  mu_set_user_email_domain (domain);
  free (domain);
  
cleanup:
  mu_address_destroy (&addr);

  return err;
}

static char *mu_user_email_domain = 0;

int
mu_set_user_email_domain (const char *domain)
{
  char* d = NULL;
  
  if (!domain)
    return EINVAL;
  
  d = strdup (domain);

  if (!d)
    return ENOMEM;

  if (mu_user_email_domain)
    free (mu_user_email_domain);

  mu_user_email_domain = d;

  return 0;
}

int
mu_get_user_email_domain (const char **domain)
{
  int err = 0;

  if (!mu_user_email_domain)
    {
      if ((err = mu_get_host_name (&mu_user_email_domain)))
	return err;
    }

  *domain = mu_user_email_domain;

  return 0;
}

/* Note: allocates memory */
char *
mu_get_user_email (const char *name)
{
  int status = 0;
  char *localpart = NULL;
  const char *domainpart = NULL;
  char *email = NULL;
  char *tmpname = NULL;

  if (!name && mu_user_email)
    {
      email = strdup (mu_user_email);
      if (!email)
	errno = ENOMEM;
      return email;
    }

  if (!name)
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());
      if (!auth)
	{
	  errno = EINVAL;
	  return NULL;
	}
      name = tmpname = strdup(auth->name);
      if (auth)
	mu_auth_data_free (auth);
    }

  status = mu_get_user_email_domain (&domainpart);

  if (status)
    {
      free(tmpname);
      errno = status;
      return NULL;
    }

  if ((status = mu_parse822_quote_local_part (&localpart, name)))
    {
      free(tmpname);
      errno = status;
      return NULL;
    }


  email = malloc (strlen (localpart) + 1
		  + strlen (domainpart) + 1);
  if (!email)
    errno = ENOMEM;
  else
    sprintf (email, "%s@%s", localpart, domainpart);

  free(tmpname);
  free (localpart);

  return email;
}

/* mu_normalize_path: convert pathname containig relative paths specs (../)
   into an equivalent absolute path. Strip trailing delimiter if present,
   unless it is the only character left. E.g.:

         /home/user/../smith   -->   /home/smith
	 /home/user/../..      -->   /

   FIXME: delim is superfluous. The function deals with unix filesystem
   paths, so delim should be always "/" */
char *
mu_normalize_path (char *path, const char *delim)
{
  int len;
  char *p;

  if (!path)
    return path;

  len = strlen (path);

  /* Empty string is returned as is */
  if (len == 0)
    return path;

  /* delete trailing delimiter if any */
  if (len && path[len-1] == delim[0])
    path[len-1] = 0;

  /* Eliminate any /../ */
  for (p = strchr (path, '.'); p; p = strchr (p, '.'))
    {
      if (p > path && p[-1] == delim[0])
	{
	  if (p[1] == '.' && (p[2] == 0 || p[2] == delim[0]))
	    /* found */
	    {
	      char *q, *s;

	      /* Find previous delimiter */
	      for (q = p-2; *q != delim[0] && q >= path; q--)
		;

	      if (q < path)
		break;
	      /* Copy stuff */
	      s = p + 2;
	      p = q;
	      while ((*q++ = *s++))
		;
	      continue;
	    }
	}

      p++;
    }

  if (path[0] == 0)
    {
      path[0] = delim[0];
      path[1] = 0;
    }

  return path;
}

int
mu_normalize_mailbox_url (char **pout, const char *dir)
{
  int len;

  if (!pout)
    return MU_ERR_OUT_PTR_NULL;
      
  len = strlen (dir);
  if (dir[len-1] == '/')
    *pout = strdup (dir);
  else if (strncasecmp (dir, "mbox:", 5) == 0 && dir[len-1] == '=')
    {
      if (len > 5 && strcmp (dir + len - 5, "user=") == 0)
	*pout = strdup (dir);
      else
	return MU_ERR_BAD_FILENAME;
    }
  else
    {
      *pout = malloc (strlen (dir) + 2);
      if (*pout)
	strcat (strcpy (*pout, dir), "/");
    }

  /* Final check */
  return (*pout == NULL) ? errno : 0;
}

/* Create and open a temporary file. Be very careful about it, since we
   may be running with extra privilege i.e setgid().
   Returns file descriptor of the open file.
   If namep is not NULL, the pointer to the malloced file name will
   be stored there. Otherwise, the file is unlinked right after open,
   i.e. it will disappear after close(fd). */

#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif

int
mu_tempfile (const char *tmpdir, char **namep)
{
  char *filename;
  int fd;

  if (!tmpdir)
    tmpdir = (getenv ("TMPDIR")) ? getenv ("TMPDIR") : P_tmpdir;

  filename = malloc (strlen (tmpdir) + /*'/'*/1 + /* "muXXXXXX" */8 + 1);
  if (!filename)
    return -1;
  sprintf (filename, "%s/muXXXXXX", tmpdir);

#ifdef HAVE_MKSTEMP
  {
    int save_mask = umask (077);
    fd = mkstemp (filename);
    umask (save_mask);
  }
#else
  if (mktemp (filename))
    fd = open (filename, O_CREAT|O_EXCL|O_RDWR, 0600);
  else
    fd = -1;
#endif

  if (fd == -1)
    {
      mu_error (_("Cannot open temporary file: %s"), strerror (errno));
      free (filename);
      return -1;
    }

  if (namep)
    *namep = filename;
  else
    {
      unlink (filename);
      free (filename);
    }

  return fd;
}

/* Create a unique temporary file name in tmpdir. The function
   creates an empty file with this name to avoid possible race
   conditions. Returns a pointer to the malloc'ed file name.
   If tmpdir is NULL, the value of the environment variable
   TMPDIR or the hardcoded P_tmpdir is used, whichever is defined. */

char *
mu_tempname (const char *tmpdir)
{
  char *filename = NULL;
  int fd = mu_tempfile (tmpdir, &filename);
  close (fd);
  return filename;
}

/* See Advanced Programming in the UNIX Environment, Stevens,
 * program  10.20 for the rational for the signal handling. I
 * had to look it up, so if somebody else is curious, thats where
 * to find it.
 */
int 
mu_spawnvp (const char *prog, char * const av[], int *stat)
{
  pid_t pid;
  int err = 0;
  int progstat;
  struct sigaction ignore;
  struct sigaction saveintr;
  struct sigaction savequit;
  sigset_t chldmask;
  sigset_t savemask;

  if (!prog || !av)
    return EINVAL;

  ignore.sa_handler = SIG_IGN;	/* ignore SIGINT and SIGQUIT */
  ignore.sa_flags = 0;
  sigemptyset (&ignore.sa_mask);

  if (sigaction (SIGINT, &ignore, &saveintr) < 0)
    return errno;
  if (sigaction (SIGQUIT, &ignore, &savequit) < 0)
    {
      sigaction (SIGINT, &saveintr, NULL);
      return errno;
    }

  sigemptyset (&chldmask);	/* now block SIGCHLD */
  sigaddset (&chldmask, SIGCHLD);

  if (sigprocmask (SIG_BLOCK, &chldmask, &savemask) < 0)
    {
      sigaction (SIGINT, &saveintr, NULL);
      sigaction (SIGQUIT, &savequit, NULL);
      return errno;
    }

#ifdef HAVE_VFORK
  pid = vfork ();
#else
  pid = fork ();
#endif

  if (pid < 0)
    {
      err = errno;
    }
  else if (pid == 0)
    {				/* child */
      /* restore previous signal actions & reset signal mask */
      sigaction (SIGINT, &saveintr, NULL);
      sigaction (SIGQUIT, &savequit, NULL);
      sigprocmask (SIG_SETMASK, &savemask, NULL);

      execvp (prog, av);
#ifdef HAVE__EXIT      
      _exit (127);		/* exec error */
#else
      exit (127);
#endif
    }
  else
    {				/* parent */
      while (waitpid (pid, &progstat, 0) < 0)
	if (errno != EINTR)
	  {
	    err = errno;	/* error other than EINTR from waitpid() */
	    break;
	  }
      if (err == 0 && stat)
	*stat = progstat;
    }

  /* restore previous signal actions & reset signal mask */
  /* preserve first error number, but still try and reset the signals */
  if (sigaction (SIGINT, &saveintr, NULL) < 0)
    err = err ? err : errno;
  if (sigaction (SIGQUIT, &savequit, NULL) < 0)
    err = err ? err : errno;
  if (sigprocmask (SIG_SETMASK, &savemask, NULL) < 0)
    err = err ? err : errno;

  return err;
}

/* The result of readlink() may be a path relative to that link, 
 * qualify it if necessary.
 */
static void
mu_qualify_link (const char *path, const char *link, char *qualified)
{
  const char *lb = NULL;
  size_t len;

  /* link is full path */
  if (*link == '/')
    {
      mu_cpystr (qualified, link, _POSIX_PATH_MAX);
      return;
    }

  if ((lb = strrchr (path, '/')) == NULL)
    {
      /* no path in link */
      mu_cpystr (qualified, link, _POSIX_PATH_MAX);
      return;
    }

  len = lb - path + 1;
  memcpy (qualified, path, len);
  mu_cpystr (qualified + len, link, _POSIX_PATH_MAX - len);
}

#ifndef _POSIX_SYMLOOP_MAX
# define _POSIX_SYMLOOP_MAX 255
#endif

int
mu_unroll_symlink (char *out, size_t outsz, const char *in)
{
  char path[_POSIX_PATH_MAX];
  int symloops = 0;

  while (symloops++ < _POSIX_SYMLOOP_MAX)
    {
      struct stat s;
      char link[_POSIX_PATH_MAX];
      char qualified[_POSIX_PATH_MAX];
      int len;

      if (lstat (in, &s) == -1)
	return errno;

      if (!S_ISLNK (s.st_mode))
	{
	  mu_cpystr (path, in, sizeof (path));
	  break;
	}

      if ((len = readlink (in, link, sizeof (link))) == -1)
	return errno;

      link[(len >= sizeof (link)) ? (sizeof (link) - 1) : len] = '\0';

      mu_qualify_link (in, link, qualified);

      mu_cpystr (path, qualified, sizeof (path));

      in = path;
    }

  mu_cpystr (out, path, outsz);

  return 0;
}

/* Expand a PATTERN to the pathname. PATTERN may contain the following
   macro-notations:
   ---------+------------ 
   notation |  expands to
   ---------+------------
   %u         user name
   %h         user's home dir
   ~          Likewise
   ---------+------------

   Allocates memory. 
*/
char *
mu_expand_path_pattern (const char *pattern, const char *username)
{
  const char *p, *startp;
  char *q;
  char *path;
  int len = 0;
  struct mu_auth_data *auth = NULL;
  
  for (p = pattern; *p; p++)
    {
      if (*p == '~')
        {
          if (!auth)
            {
              auth = mu_get_auth_by_name (username);
              if (!auth)
                return NULL;
            }
          len += strlen (auth->dir);
        }
      else if (*p == '%')
	switch (*++p)
	  {
	  case 'u':
	    len += strlen (username);
	    break;
	    
	  case 'h':
	    if (!auth)
	      {
		auth = mu_get_auth_by_name (username);
		if (!auth)
		  return NULL;
	      }
	    len += strlen (auth->dir);
	    break;
	    
	  case '%':
	    len++;
	    break;
	    
	  default:
	    len += 2;
	  }
      else
	len++;
    }
  
  path = malloc (len + 1);
  if (!path)
    return NULL;

  startp = pattern;
  q = path;
  while (*startp && (p = strchr (startp, '%')) != NULL)
    {
      memcpy (q, startp, p - startp);
      q += p - startp;
      switch (*++p)
	{
	case 'u':
	  strcpy (q, username);
	  q += strlen (username);
	  break;
	  
	case 'h':
	  strcpy (q, auth->dir);
	  q += strlen (auth->dir);
	  break;
	  
	case '%':
	  *q++ = '%';
	  break;
	  
	default:
	  *q++ = '%';
	  *q++ = *p;
	}
      startp = p + 1;
    }
  if (*startp)
    {
      strcpy (q, startp);
      q += strlen (startp);
    }
  *q = 0;
  if (auth)
    mu_auth_data_free (auth);
  return path;
}

#define ST_INIT  0
#define ST_MSGID 1

static int
strip_message_id (char *msgid, char **pval)
{
  char *p, *q;
  int state;
  
  *pval = strdup (msgid);
  if (!*pval)
    return ENOMEM;
  state = ST_INIT;
  for (p = q = *pval; *p; p++)
    {
      switch (state)
	{
	case ST_INIT:
	  if (*p == '<')
	    {
	      *q++ = *p;
	      state = ST_MSGID;
	    }
	  else if (isspace (*p))
	    *q++ = *p;
	  break;

	case ST_MSGID:
	  *q++ = *p;
	  if (*p == '>')
	    state = ST_INIT;
	  break;
	}
    }
  *q = 0;
  return 0;
}

int
get_msgid_header (mu_header_t hdr, const char *name, char **val)
{
  char *p;
  int status = mu_header_aget_value (hdr, name, &p);
  if (status)
    return status;
  status = strip_message_id (p, val);
  free (p);
  return status;
}

static char *
concat (const char *s1, const char *s2)
{
  int len = (s1 ? strlen (s1) : 0) + (s2 ? strlen (s2) : 0) + 2;
  char *s = malloc (len);
  if (s)
    {
      char *p = s;
      
      if (s1)
	{
	  strcpy (p, s1);
	  p += strlen (s1);
	  *p++ = ' ';
	}
      if (s2)
	strcpy (p, s2);
    }
  return s;
}

/* rfc2822:
   
   The "References:" field will contain the contents of the parent's
   "References:" field (if any) followed by the contents of the parent's
   "Message-ID:" field (if any).  If the parent message does not contain
   a "References:" field but does have an "In-Reply-To:" field
   containing a single message identifier, then the "References:" field
   will contain the contents of the parent's "In-Reply-To:" field
   followed by the contents of the parent's "Message-ID:" field (if
   any).  If the parent has none of the "References:", "In-Reply-To:",
   or "Message-ID:" fields, then the new message will have no
   References:" field. */

int
mu_rfc2822_references (mu_message_t msg, char **pstr)
{
  char *ref = NULL, *msgid = NULL;
  mu_header_t hdr;
  int rc;
  
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    return rc;
  get_msgid_header (hdr, MU_HEADER_MESSAGE_ID, &msgid);
  if (get_msgid_header (hdr, MU_HEADER_REFERENCES, &ref))
    get_msgid_header (hdr, MU_HEADER_IN_REPLY_TO, &ref);

  if (ref || msgid)
    {
      *pstr = concat (ref, msgid);
      free (ref);
      free (msgid);
      return 0;
    }
  return MU_ERR_FAILURE;
}

#define DATEBUFSIZE 128
#define COMMENT "Your message of "

/*
   The "In-Reply-To:" field will contain the contents of the "Message-
   ID:" field of the message to which this one is a reply (the "parent
   message").  If there is more than one parent message, then the "In-
   Reply-To:" field will contain the contents of all of the parents'
   "Message-ID:" fields.  If there is no "Message-ID:" field in any of
   the parent messages, then the new message will have no "In-Reply-To:"
   field.
*/
int
mu_rfc2822_in_reply_to (mu_message_t msg, char **pstr)
{
  char *value, *s1 = NULL, *s2 = NULL;
  mu_header_t hdr;
  int rc;
  
  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    return rc;
  
  if (mu_header_aget_value (hdr, MU_HEADER_DATE, &value))
    {
      mu_envelope_t envelope = NULL;
      value = malloc (DATEBUFSIZE);
      if (value)
	{
	  mu_message_get_envelope (msg, &envelope);
	  mu_envelope_date (envelope, value, DATEBUFSIZE, NULL);
	}
    }

  if (value)
    {
      s1 = malloc (sizeof (COMMENT) + strlen (value));
      if (s1)
	strcat (strcpy (s1, COMMENT), value);
      free (value);
      if (!s1)
	return ENOMEM;
    }
  
  if (mu_header_aget_value (hdr, MU_HEADER_MESSAGE_ID, &value) == 0)
    {
      s2 = malloc (strlen (value) + 3);
      if (s2)
	strcat (strcpy (s2, "\n\t"), value);
	  
      free (value);
      if (!s2)
	{
	  free (s1);
	  return ENOMEM;
	}
    }

  if (s1 || s2)
    {
      *pstr = concat (s1, s2);
      free (s1);
      free (s2);
      return 0;
    }
  return MU_ERR_FAILURE;
}

/* Based on strstr from GNU libc (Stephen R. van den Berg,
   berg@pool.informatik.rwth-aachen.de) */

char *
mu_strcasestr (const char *a_haystack, const char *a_needle)
{
  register const unsigned char *haystack = (unsigned char*) a_haystack,
    *needle = (unsigned char*) a_needle;
  register unsigned int b, c;

#define U(c) toupper (c)
  if ((b = U (*needle)))
    {
      haystack--;		
      do
	{
	  if (!(c = *++haystack))
	    goto ret0;
	}
      while (U (c) != b);

      if (!(c = *++needle))
	goto foundneedle;

      c = U (c);
      ++needle;
      goto jin;

      for (;;)
        { 
          register unsigned int a;
	  register const unsigned char *rhaystack, *rneedle;

	  do
	    {
	      if (!(a = *++haystack))
		goto ret0;
	      if (U (a) == b)
		break;
	      if (!(a = *++haystack))
		goto ret0;
shloop:       ;
            }
          while (U (a) != b);
	  
jin:     if (!(a = *++haystack))
	    goto ret0;

	  if (U (a) != c)
	    goto shloop;

	  if (U (*(rhaystack = haystack-- + 1)) ==
	      (a = U (*(rneedle = needle))))
	    do
	      {
		if (!a)
		  goto foundneedle;
		if (U (*++rhaystack) != (a = U (*++needle)))
		  break;
		if (!a)
		  goto foundneedle;
	      }
	    while (U (*++rhaystack) == (a = U (*++needle)));

	  needle = rneedle;

	  if (!a)
	    break;
        }
    }
foundneedle:
  return (char*)haystack;
ret0:
  return NULL;

#undef U
}  

int
mu_string_unfold (char *text, size_t *plen)
{
  char *p, *q;

  if (!text)
    return EINVAL;
  
  for (p = q = text; *q; q++)
    if (*q != '\n')
      *p++ = *q;
  *p++ = 0;
  if (plen)
    *plen = p - text;
  return 0;
}

int
mu_true_answer_p (const char *p)
{
  if (!p)
    return -1;

  while (*p && isspace (*p))
    p++;

  if (*p)
    {
      /* TRANSLATORS: This is a list of characters which start
	 an affirmative answer. Whenever possible, please preserve
	 'yY' in your translation, e.g., for Euskara:

	 msgstr "yYbB";
      */
      if (strchr (_("yY"), *p))
	return 1;

      /* TRANSLATORS: This is a list of characters which start
	 a negative answer. Whenever possible, please preserve
	 'nN' in your translation, e.g., for Euskara:

	 msgstr "nNeE";
      */
      else if (strchr (_("nN"), *p))
	return 0;
    }
  return -1;
}

/* Returns true if SCHEME represents a local mail folder. Stores
   real folder path to PATH */
int
mu_scheme_autodetect_p (const char *scheme,  const char **path)
{
  *path = scheme;
  if (strncmp (MU_FILE_SCHEME, scheme, MU_FILE_SCHEME_LEN) == 0)
    {
      *path += MU_FILE_SCHEME_LEN;
      return 1;
    }
  if (strncmp (MU_PATH_SCHEME, scheme, MU_PATH_SCHEME_LEN) == 0)
    return 1;
  return 0;
}
    
int
mu_fd_wait (int fd, int *pflags, struct timeval *tvp)
{
  fd_set rdset, wrset;
  int rc;

  FD_ZERO (&rdset);
  FD_ZERO (&wrset);
  if ((*pflags) & MU_STREAM_READY_RD)
    FD_SET (fd, &rdset);
  if ((*pflags) & MU_STREAM_READY_WR)
    FD_SET (fd, &wrset);
  
  do
    {
      if (tvp)
	{
	  struct timeval tv = *tvp; 
	  rc = select (fd + 1, &rdset, &wrset, NULL, &tv);
	}
      else
	rc = select (fd + 1, &rdset, &wrset, NULL, NULL);
    }
  while (rc == -1 && errno == EINTR);

  if (rc < 0)
    return errno;
  
  *pflags = 0;
  if (rc > 0)
    {
      if (FD_ISSET (fd, &rdset))
	*pflags |= MU_STREAM_READY_RD;
      if (FD_ISSET (fd, &wrset))
	*pflags |= MU_STREAM_READY_WR;
    }
  return 0;
}

enum mu_iconv_fallback_mode mu_default_fallback_mode = mu_fallback_copy_octal;

int
mu_set_default_fallback (const char *str)
{
  if (strcmp (str, "none") == 0)
    mu_default_fallback_mode = mu_fallback_none;
  else if (strcmp (str, "copy-pass") == 0)
    mu_default_fallback_mode = mu_fallback_copy_pass;
  else if (strcmp (str, "copy-octal") == 0)
    mu_default_fallback_mode = mu_fallback_copy_octal;
  else
    return EINVAL;
  return 0;
}

int
mu_decode_filter (mu_stream_t *pfilter, mu_stream_t input,
		  const char *filter_type,
		  const char *fromcode, const char *tocode)
{
  mu_stream_t filter;
  
  int status = mu_filter_create (&filter, input, filter_type,
			      MU_FILTER_DECODE, MU_STREAM_READ);
  if (status)
    return status;

  if (fromcode && tocode && strcasecmp (fromcode, tocode))
    {
      mu_stream_t cvt;
      status = mu_filter_iconv_create (&cvt, filter, fromcode, tocode,
				    MU_STREAM_NO_CLOSE,
				    mu_default_fallback_mode);
      if (status == 0)
	{
	  if (mu_stream_open (cvt))
	    mu_stream_destroy (&cvt, mu_stream_get_owner (cvt));
	  else
	    {
	      int flags;
	      mu_stream_get_flags (cvt, &flags);
	      flags &= ~MU_STREAM_NO_CLOSE;
	      mu_stream_set_flags (cvt, flags);
	      filter = cvt;
	    }
	}
    }
  *pfilter = filter;
  return 0;
}

int
mu_is_proto (const char *p)
{
  for (; *p && *p != '/'; p++)
    if (*p == ':')
      return 1;
  return 0;
}

int
mu_mh_delim (const char *str)
{
  if (str[0] == '-')
    {
      for (; *str == '-'; str++)
	;
      for (; *str == ' ' || *str == '\t'; str++)
	;
    }
  return str[0] == '\n';
}

char *
__argp_base_name (const char *arg)
{
  char *p = strrchr (arg, '/');
  return (char *)(p ? p + 1 : arg);
}

/* A locale-independent version of strftime */
size_t
mu_strftime (char *s, size_t max, const char *format, const struct tm *tm)
{
  size_t size;
  mu_set_locale ("C");
  size = strftime(s, max, format, tm);
  mu_restore_locale ();
  return size;
}
  
    

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <mailutils/address.h>
#include <mailutils/error.h>
#include <mailutils/iterator.h>
#include <mailutils/mutil.h>
#include <mailutils/parse822.h>
#include <mailutils/mu_auth.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/envelope.h>

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
      struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());
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
  char *p = strdup (ref);
  char *home = NULL;

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
		return NULL;
	      homedir = home;
	    }
	  s = calloc (strlen (homedir) + strlen (p) + 1, 1);
          strcpy (s, homedir);
          strcat (s, p);
          free (--p);
          p = s;
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
          name [s - p] = '\0';
	  
          auth = mu_get_auth_by_name (name);
          free (name);
          if (auth)
            {
              char *buf = calloc (strlen (auth->dir) + strlen (s) + 1, 1);
	      strcpy (buf, auth->dir);
              strcat (buf, s);
              free (--p);
              p = buf;
	      mu_auth_data_free (auth);
            }
          else
            p--;
        }
    }
  if (home)
    free (home);
  return p;
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
mu_register_retriever (list_t *pflist, mu_retrieve_fp fun)
{
  if (!*pflist && list_create (pflist))
    return;
  list_append (*pflist, fun);
}

void *
mu_retrieve (list_t flist, void *data)
{
  void *p = NULL;
  iterator_t itr;

  if (iterator_create (&itr, flist) == 0)
    {
      mu_retrieve_fp fun;
      for (iterator_first (itr); !p && !iterator_is_done (itr);
	   iterator_next (itr))
	{
	  iterator_current (itr, (void **)&fun);
	  p = (*fun) (data);
	}

      iterator_destroy (&itr);
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
  address_t addr = NULL;
  size_t emailno = 0;
  char *email = NULL;
  char *domain = NULL;
  
  if ((err = address_create (&addr, candidate)) != 0)
    return err;

  if ((err = address_get_email_count (addr, &emailno)) != 0)
    goto cleanup;

  if (emailno != 1)
    {
      errno = EINVAL;
      goto cleanup;
    }

  if ((err = address_aget_email (addr, 1, &email)) != 0)
    goto cleanup;

  if (mu_user_email)
    free (mu_user_email);

  mu_user_email = email;

  address_aget_domain (addr, 1, &domain);
  mu_set_user_email_domain (domain);
  free (domain);
  
cleanup:
  address_destroy (&addr);

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

  if (!name && mu_user_email)
    {
      email = strdup (mu_user_email);
      if (!email)
	errno = ENOMEM;
      return email;
    }

  if (!name)
    {
      struct passwd *pw = getpwuid (getuid ());
      if (!pw)
	{
	  errno = EINVAL;
	  return NULL;
	}
      name = pw->pw_name;
    }

  status = mu_get_user_email_domain (&domainpart);

  if (status)
    {
      errno = status;
      return NULL;
    }

  if ((status = parse822_quote_local_part (&localpart, name)))
    {
      errno = status;
      return NULL;
    }


  email = malloc (strlen (localpart) + 1
		  + strlen (domainpart) + 1);
  if (!email)
    errno = ENOMEM;
  else
    sprintf (email, "%s@%s", localpart, domainpart);

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

char *
mu_normalize_maildir (const char *dir)
{
  int len = strlen (dir);
  if (dir[len-1] == '/')
    return strdup (dir);
  else if (strncasecmp (dir, "mbox:", 5) == 0 && dir[len-1] == '=')
    {
      if (len > 5 && strcmp (dir + len - 5, "user=") == 0)
	return strdup (dir);
      else
	return NULL;
    }
  else
    {
      char *p = malloc (strlen (dir) + 2);
      strcat (strcpy (p, dir), "/");
      return p;
    }
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
      mu_error ("Can not open temporary file: %s", strerror(errno));
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
mu_spawnvp (const char* prog, const char* const av_[], int* stat)
{
  pid_t pid;
  int err = 0;
  int progstat;
  struct sigaction ignore;
  struct sigaction saveintr;
  struct sigaction savequit;
  sigset_t chldmask;
  sigset_t savemask;
  char** av = (char**) av_;

  if (!prog || !av)
    return EINVAL;

  ignore.sa_handler = SIG_IGN;	/* ignore SIGINT and SIGQUIT */
  ignore.sa_flags = 0;
  sigemptyset (&ignore.sa_mask);

  if (sigaction (SIGINT, &ignore, &saveintr) < 0)
    return errno;
  if (sigaction (SIGQUIT, &ignore, &savequit) < 0)
    return errno;

  sigemptyset (&chldmask);	/* now block SIGCHLD */
  sigaddset (&chldmask, SIGCHLD);

  if (sigprocmask (SIG_BLOCK, &chldmask, &savemask) < 0)
    return errno;

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

      execvp (av[0], av);
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
      if(err == 0 && stat)
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
      if (*p == '%')
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
get_msgid_header (header_t hdr, const char *name, char **val)
{
  char *p;
  int status = header_aget_value (hdr, name, &p);
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
mu_rfc2822_references (message_t msg, char **pstr)
{
  char *ref = NULL, *msgid = NULL;
  header_t hdr;
  int rc;
  
  rc = message_get_header (msg, &hdr);
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
    }
  return 0;
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
mu_rfc2822_in_reply_to (message_t msg, char **pstr)
{
  char *value, *s1 = NULL, *s2 = NULL;
  header_t hdr;
  int rc;
  
  rc = message_get_header (msg, &hdr);
  if (rc)
    return rc;
  
  if (header_aget_value (hdr, MU_HEADER_DATE, &value))
    {
      envelope_t envelope = NULL;
      value = malloc (DATEBUFSIZE);
      if (value)
	{
	  message_get_envelope (msg, &envelope);
	  envelope_date (envelope, value, DATEBUFSIZE, NULL);
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
  
  if (header_aget_value (hdr, MU_HEADER_MESSAGE_ID, &value) == 0)
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
    }
  return 0;
}

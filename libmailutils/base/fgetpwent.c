/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012, 2014 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Assuming FP refers to an open file in format of /etc/passwd, read the next
   line and store a pointer to a structure containing the broken-out
   fields of the record in the location pointed to by RESULT.  Use *BUFP
   (of size *BUFS) to store the data, reallocating it if necessary.  If
   *BUFP is NULL or *BUFS is 0, allocate new memory.  The caller should
   free *BUFP when no longer needed.  Do not try to free *RESULT!

   Input lines not conforming to the /etc/passwd format are silently
   ignored.
   
   Return value:
    0      - success
    ENOMEM - not enough memory
    ENOENT - no more entries.
*/

int
mu_fgetpwent_r (FILE *fp, char **bufp, size_t *bufs, struct passwd **result)
{
  char *buffer = *bufp;
  size_t buflen = *bufs;
  struct passwd *pwbuf;
  size_t pos = 0;
  int c;
  size_t off[6];
  int i = 0;

  if (!buffer)
    buflen = 0;
  
  while ((c = fgetc(fp)) != EOF)
    {
      if (pos == buflen)
	{
	  char *nb;
	  size_t ns;
	  
	  if (buflen == 0)
	    ns = 128;
	  else
	    {
	      ns = ns * 2;
	      if (ns < buflen)
		return ENOMEM;
	    }
	  nb = realloc(buffer, ns);
	  if (!nb)
	    return ENOMEM;
	  buffer = nb;
	  buflen = ns;
	}
      if (c == '\n')
	{
	  buffer[pos++] = 0;
	  if (i != sizeof(off)/sizeof(off[0]))
	    continue;
	  break;
	}
      if (c == ':')
	{
	  buffer[pos++] = 0;
	  if (i < sizeof(off)/sizeof(off[0]))
	    off[i++] = pos;
	}
      else
	buffer[pos++] = c;
    }

  if (pos == 0)
    return ENOENT;

  if (pos + sizeof(struct passwd) > buflen)
    {
      char *nb;
      size_t ns;
	  
      ns = pos + sizeof(struct passwd);
      if (ns < buflen)
	return ENOMEM;
    
      nb = realloc(buffer, ns);
      if (!nb)
	return ENOMEM;
      buffer = nb;
      buflen = ns;
    }
  pwbuf = (struct passwd*)((char*) buffer + pos);
  
  pwbuf->pw_name   = buffer;
  pwbuf->pw_passwd = buffer + off[0];
  pwbuf->pw_uid    = strtoul(buffer + off[1], NULL, 10);
  pwbuf->pw_gid    = strtoul(buffer + off[2], NULL, 10);
  pwbuf->pw_gecos  = buffer + off[3];
  pwbuf->pw_dir    = buffer + off[4];
  pwbuf->pw_shell  = buffer + off[5];

  *bufp   = buffer;
  *bufs   = buflen;
  *result = pwbuf;
  return 0;
}

/* A simple replacement for fgetpwent().
   Note:
    - it is not thread safe (neither is the original fgetpwent)
    - uses dynamically allocated buffer that is __never__ freed.
    - no support for shadow nor BSD-style pwddb (neither has the original
    fgetpwent).
    - no support for NIS(+).

   Initial implementation by Alain Magloire.
   Rewritten from scratch by Sergey Poznyakoff.
*/
struct passwd *
mu_fgetpwent (FILE *fp)
{
  static char *buffer;
  static size_t bufsize;
  static struct passwd *pwbuf;
  int rc = mu_fgetpwent_r (fp, &buffer, &bufsize, &pwbuf);
  if (rc)
    {
      errno = rc;
      return NULL;
    }
  return pwbuf;
}

#ifdef STANDALONE
# include <assert.h>

int
main (int argc, char **argv)
{
  FILE *fp;
  
  if (argc == 1)
    {
      char *a[3];
      a[0] = argv[0];
      a[1] = "/etc/passwd";
      a[2] = NULL;
      argv = a;
      argc = 2;
    }
  while (--argc)
    {
      struct passwd *pwd;
      char *file = *++argv;

      fp = fopen (file, "r");
      if (!fp)
	{
	  perror (file);
	  continue;
	}
      printf ("Reading %s\n", file);
      while ((pwd = fgetpwent (fp)))
        {
          printf ("--------------------------------------\n");
          printf ("name %s\n", pwd->pw_name);
          printf ("passwd %s\n", pwd->pw_passwd);
          printf ("uid %d\n", pwd->pw_uid);
          printf ("gid %d\n", pwd->pw_gid);
          printf ("gecos %s\n", pwd->pw_gecos);
          printf ("dir %s\n", pwd->pw_dir);
          printf ("shell %s\n", pwd->pw_shell);
        }
      printf ("======================================\n");
      printf ("End of %s\n", file);
      close (fp);
    }
  return 0;
}

#endif

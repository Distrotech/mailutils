/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <mailutils/wordsplit.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/assoc.h>
#include <mailutils/stream.h>
#include <mailutils/sql.h>

int
mu_is_proto (const char *p)
{
  if (*p == '|')
    return 1;
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

static void
assoc_str_free (void *data)
{
  free (data);
}

int
mutil_parse_field_map (const char *map, mu_assoc_t *passoc_tab, int *perr)
{
  int rc;
  int i;
  struct mu_wordsplit ws;
  mu_assoc_t assoc_tab = NULL;

  ws.ws_delim = ":";
  if (mu_wordsplit (map, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM))
    {
      mu_error (_("cannot split line `%s': %s"), map,
		mu_wordsplit_strerror (&ws));
      return errno;
    }

  for (i = 0; i < ws.ws_wordc; i++)
    {
      char *tok = ws.ws_wordv[i];
      char *p = strchr (tok, '=');
      char *pptr;
      
      if (!p)
	{
	  rc = EINVAL;
	  break;
	}
      if (!assoc_tab)
	{
	  rc = mu_assoc_create (&assoc_tab, sizeof(char*), 0);
	  if (rc)
	    break;
	  mu_assoc_set_free (assoc_tab, assoc_str_free);
	  *passoc_tab = assoc_tab;
	}
      *p++ = 0;
      pptr = strdup (p);
      if (!pptr)
	{
	  rc = errno;
	  break;
	}
      rc = mu_assoc_install (assoc_tab, tok, &pptr);
      if (rc)
	{
	  free (p);
	  break;
	}
    }

  mu_wordsplit_free (&ws);
  if (rc && perr)
    *perr = i;
  return rc;
}

/* FIXME: should it be here? */
int
mu_sql_decode_password_type (const char *arg, enum mu_password_type *t)
{
  if (strcmp (arg, "plain") == 0)
    *t = password_plaintext;
  else if (strcmp (arg, "hash") == 0)
    *t = password_hash;
  else if (strcmp (arg, "scrambled") == 0)
    *t = password_scrambled;
  else
    return 1;
  return 0;
}

int
mu_stream_flags_to_mode (int flags, int isdir)
{
  int mode = 0;
  if (flags & MU_STREAM_IRGRP)
    mode |= S_IRGRP;
  if (flags & MU_STREAM_IWGRP)
    mode |= S_IWGRP;
  if (flags & MU_STREAM_IROTH)
    mode |= S_IROTH;
  if (flags & MU_STREAM_IWOTH)
    mode |= S_IWOTH;

  if (isdir)
    {
      if (mode & (S_IRGRP|S_IWGRP))
	mode |= S_IXGRP;
      if (mode & (S_IROTH|S_IWOTH))
	mode |= S_IXOTH;
    }
  
  return mode;
}

/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * sieve header field cache.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "svfield.h"

static int
sv_hash_field_name (const char *name)
{
  int x = 0;
  /* all CHAR up to the first that is ' ', :, or a ctrl char */
  for (; !iscntrl (*name) && (*name != ' ') && (*name != ':'); name++)
    {
      x *= 256;
      x += tolower (*name);
      x %= SV_FIELD_CACHE_SIZE;
    }
  return x;
}

/*
The body is kept, the name is still the caller's.
*/
int
sv_field_cache_add (sv_field_cache_t * m, const char *name, char *body)
{
  int cl, clinit;

  /* put it in the hash table */
  clinit = cl = sv_hash_field_name (name);

  while (m->cache[cl] != NULL && strcasecmp (name, m->cache[cl]->name))
    {
      cl++;			/* resolve collisions linearly */
      cl %= SV_FIELD_CACHE_SIZE;
      if (cl == clinit)
	break;			/* gone all the way around, so bail */
    }

  /* found where to put it, so insert it into a list */
  if (m->cache[cl])
    {
      /* add this body on */

      m->cache[cl]->contents[m->cache[cl]->ncontents++] = body;

      /* whoops, won't have room for the null at the end! */
      if (!(m->cache[cl]->ncontents % 8))
	{
	  /* increase the size */
	  sv_field_t* field = (sv_field_t *) realloc (
		  m->cache[cl],
		  sizeof(sv_field_t) +
		    ((8 + m->cache[cl]->ncontents) * sizeof(char *))
		  );
	  if (field == NULL)
	    {
	      return ENOMEM;
	    }
	  m->cache[cl] = field;
	}
    }
  else
    {
      char* n = 0;
      /* create a new entry in the hash table */
      sv_field_t* field = (sv_field_t *) malloc (sizeof (sv_field_t) +
						8 * sizeof (char *));
      if (!field)
	return ENOMEM;

      n = strdup (name);

      if(!n)
      {
	free(field);
	return ENOMEM;
      }
	
      m->cache[cl] = field;
      m->cache[cl]->name = n;
      m->cache[cl]->contents[0] = body;
      m->cache[cl]->ncontents = 1;
    }

  /* we always want a NULL at the end */
  m->cache[cl]->contents[m->cache[cl]->ncontents] = NULL;

  return 0;
}

int
sv_field_cache_get (sv_field_cache_t * m,
		    const char *name, const char ***body)
{
  int rc = ENOENT;
  int clinit, cl;

  clinit = cl = sv_hash_field_name (name);

  while (m->cache[cl] != NULL)
    {
      if (strcasecmp (name, m->cache[cl]->name) == 0)
	{
	  *body = (const char **) m->cache[cl]->contents;
	  rc = 0;
	  break;
	}
      cl++;			/* try next hash bin */
      cl %= SV_FIELD_CACHE_SIZE;
      if (cl == clinit)
	break;			/* gone all the way around */
    }

  return rc;
}

void
sv_field_cache_release (sv_field_cache_t * m)
{
  int i;
  for (i = 0; i < SV_FIELD_CACHE_SIZE; i++)
    {
      if (m->cache[i])
	{
	  int j;
	  for (j = 0; m->cache[i]->contents[j]; j++)
	    {
	      free (m->cache[i]->contents[j]);
	    }
	  free (m->cache[i]);
	}
    }
}


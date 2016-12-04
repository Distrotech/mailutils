/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2008, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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
#include <stdlib.h>
#include <sieve-priv.h>

struct memory_cell
{
  void *ptr;
  void (*reclaim) (void *);
};

static void
memory_cell_destroy (void *item)
{
  struct memory_cell *mcp = item;
  if (!mcp->reclaim)
    abort ();
  else
    mcp->reclaim (mcp->ptr);
  free (mcp);
}

static int
memory_cell_cmp (const void *a, const void *b)
{
  struct memory_cell const *ma = a;
  struct memory_cell const *mb = b;
  return ma->ptr != mb->ptr;
}

void
mu_sieve_register_memory (mu_sieve_machine_t mach, void *ptr,
			  mu_sieve_reclaim_t reclaim)
{
  struct memory_cell *mcp;

  if (!reclaim)
    reclaim = mu_sieve_reclaim_default;
  
  if (!mach->memory_pool)
    {
      if (mu_list_create (&mach->memory_pool))
	mu_sieve_abort (mach);
      mu_list_set_destroy_item (mach->memory_pool, memory_cell_destroy);
      mu_list_set_comparator (mach->memory_pool, memory_cell_cmp);
    }
  mcp = malloc (sizeof (*mcp));
  if (!mcp)
    mu_sieve_abort (mach);
  mcp->ptr = ptr;
  mcp->reclaim = reclaim;
  if (mu_list_append (mach->memory_pool, mcp))
    {
      memory_cell_destroy (mcp);
      mu_sieve_abort (mach);
    }
}

void
mu_sieve_free (mu_sieve_machine_t mach, void *ptr)
{
  int rc;
  struct memory_cell mcell;

  if (!ptr)
    return;
  
  mcell.ptr = ptr;
  rc = mu_list_remove (mach->memory_pool, &mcell);
  if (rc)
    {
      mu_sieve_error (mach, _("INTERNAL ERROR: trying to free unregistered memory pointer"));
      abort ();
    }
}

void *
mu_sieve_alloc_memory (mu_sieve_machine_t mach, size_t size,
		       mu_sieve_reclaim_t recfun)
{
  char *p = malloc (size);
  if (!p)
    {
      mu_sieve_error (mach, "%s", mu_strerror (errno));
      mu_sieve_abort (mach);
    }
  mu_sieve_register_memory (mach, p, recfun);
  return p;
}

void *
mu_sieve_malloc (mu_sieve_machine_t mach, size_t size)
{
  return mu_sieve_alloc_memory (mach, size, mu_sieve_reclaim_default);
}

void *
mu_sieve_calloc (mu_sieve_machine_t mach, size_t nmemb, size_t size)
{
  char *p = calloc (nmemb, size);
  if (!p)
    {
      mu_sieve_error (mach, "%s", mu_strerror (errno));
      mu_sieve_abort (mach);
    }
  mu_sieve_register_memory (mach, p, mu_sieve_reclaim_default);
  return p;
}

char *
mu_sieve_strdup (mu_sieve_machine_t mach, char const *str)
{
  size_t len;
  char *p;
  
  if (!str)
    return NULL;
  len = strlen (str);
  p = mu_sieve_malloc (mach, len + 1);
  memcpy (p, str, len);
  p[len] = 0;
  return p;
} 

void *
mu_sieve_realloc (mu_sieve_machine_t mach, void *ptr, size_t size)
{
  int rc;
  struct memory_cell mcell, *mcp;

  if (!ptr)
    return mu_sieve_malloc (mach, size);
  
  mcell.ptr = ptr;
  rc = mu_list_locate (mach->memory_pool, &mcell, (void **)&mcp);
  if (rc == MU_ERR_NOENT)
    {
      mu_sieve_error (mach, _("INTERNAL ERROR: trying to reallocate unregistered memory pointer"));
      abort ();
    }
  else if (rc)
    {
      mu_sieve_error (mach, _("error reallocating memory: %s"),
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }

  ptr = realloc (mcp->ptr, size);
  if (!ptr)
    {
      mu_sieve_error (mach, _("error reallocating memory: %s"),
		      mu_strerror (errno));
      mu_sieve_abort (mach);
    }
  mcp->ptr = ptr;

  return ptr;
}

void 
mu_sieve_reclaim_default (void *p)
{
  free (p);
}

void
mu_sieve_reclaim_value (void *p)
{
  if (!p)
    return;
  /* For now, the same as _default.  Will change in the future */
  free (p);
}

/* Based on gnulib's x2nrealloc */
void
mu_i_sv_2nrealloc (mu_sieve_machine_t mach, void **pptr, size_t *pnmemb,
		   size_t size)
{
  void *ptr = *pptr;
  size_t nmemb = *pnmemb;
  
  if (!ptr)
    {
      if (!nmemb)
	{
	  /* Initial allocation size */
	  nmemb = 16;
	}
    }
  else
    {
      /* Set NMEMB = floor (1.5 * NMEMB) + 1 so that progress is made even
	 if NMEMB == 0.
	 Check for overflow, so that NMEMB * SIZE stays in size_t range.
	 The check may be slightly conservative, but an exact check isn't
	 worth the trouble.  */
      if ((size_t) -1 / 3 * 2 / size <= nmemb)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus,
			    _("requested too much memory %zu * %zu"),
			    nmemb, size);
	  mu_sieve_abort (mach);
	}
      nmemb += nmemb / 2 + 1;
    }

  ptr = mu_sieve_realloc (mach, ptr, nmemb * size);
  
  *pptr = ptr;
  *pnmemb = nmemb;
}

char *
mu_i_sv_id_canon (mu_sieve_machine_t mach, char const *name)
{
  size_t i;
  char *p;
  
  if (!name)
    return NULL;
  
  for (i = 0; i < mach->idcount; i++)
    {
      if (strcmp (mach->idspace[i], name) == 0)
	return mach->idspace[i];
    }

  if (mach->idcount == mach->idmax)
    {
      mu_i_sv_2nrealloc (mach, 
			 (void **) &mach->idspace,
			 &mach->idmax,
			 sizeof mach->idspace[0]);
    }

  p = mu_sieve_strdup (mach, name);
  mach->idspace[mach->idcount++] = p;

  return p;
}
 
size_t
mu_i_sv_id_num (mu_sieve_machine_t mach, char const *name)
{
  size_t i;

  for (i = 0; i < mach->idcount; i++)
    {
      if (mach->idspace[i] == name || strcmp (mach->idspace[i], name) == 0)
	return i;
    }
  abort ();
}

char *
mu_i_sv_id_str (mu_sieve_machine_t mach, size_t n)
{
  if (n >= mach->idcount)
    abort ();
  return mach->idspace[n];
}

void
mu_i_sv_free_idspace (mu_sieve_machine_t mach)
{
  size_t i;

  for (i = 0; i < mach->idcount; i++)
    mu_sieve_free (mach, mach->idspace[i]);
  mach->idcount = 0;
}

      

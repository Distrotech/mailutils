/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2005, 2006 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "mail.h"

static void alias_print (char *name);
static void alias_print_group (char *name, mu_list_t list);
static int  alias_create (char *name, mu_list_t *plist);
static int  alias_lookup (char *name, mu_list_t *plist);

/*
 * a[lias] [alias [address...]]
 * g[roup] [alias [address...]]
 */

int
mail_alias (int argc, char **argv)
{
  if (argc == 1)
    alias_print (NULL);
  else if (argc == 2)
    alias_print (argv[1]);
  else
    {
      mu_list_t list;

      if (alias_create (argv[1], &list))
	return 1;

      argc --;
      argv ++;
      while (--argc)
	util_slist_add (&list, *++argv);
    }
  return 0;
}

typedef struct _alias alias_t;

struct _alias
{
  char *name;
  mu_list_t list;
};

/* Hash sizes. These are prime numbers, the distance between each
   pair of them grows exponentially, starting from 64.
   Hopefully no one will need more than 32797 aliases, and even if
   someone will, it is easy enough to add more numbers to the sequence. */
static unsigned int hash_size[] =
{
  37,   101,  229,  487, 1009, 2039, 4091, 8191, 16411, 32797,
};
/* Maximum number of re-hashes: */
static unsigned int max_rehash = sizeof (hash_size) / sizeof (hash_size[0]);
static alias_t *aliases; /* Table of aliases */
static unsigned int hash_num;  /* Index to hash_size table */

static unsigned int hash (char *name);
static int alias_rehash (void);
static alias_t *alias_lookup_or_install (char *name, int install);
static void alias_print_group (char *name, mu_list_t list);

unsigned
hash (char *name)
{
    unsigned i;

    for (i = 0; *name; name++) {
        i <<= 1;
        i ^= *(unsigned char*) name;
    }
    return i % hash_size[hash_num];
}

int
alias_rehash ()
{
  alias_t *old_aliases = aliases;
  alias_t *ap;
  unsigned int i;

  if (++hash_num >= max_rehash)
    {
      util_error (_("alias hash table full"));
      return 1;
    }

  aliases = xcalloc (hash_size[hash_num], sizeof (aliases[0]));
  if (old_aliases)
    {
      for (i = 0; i < hash_size[hash_num-1]; i++)
	{
	  if (old_aliases[i].name)
	    {
	      ap = alias_lookup_or_install (old_aliases[i].name, 1);
	      ap->name = old_aliases[i].name;
	      ap->list = old_aliases[i].list;
	    }
	}
      free (old_aliases);
    }
  return 0;
}

alias_t *
alias_lookup_or_install (char *name, int install)
{
  unsigned i, pos;

  if (!aliases)
    {
      if (install)
	{
	  if (alias_rehash ())
	    return NULL;
	}
      else
	return NULL;
    }

  pos = hash (name);

  for (i = pos; aliases[i].name;)
    {
      if (strcmp(aliases[i].name, name) == 0)
	return &aliases[i];
      if (++i >= hash_size[hash_num])
	i = 0;
      if (i == pos)
	break;
    }

  if (!install)
    return NULL;

  if (aliases[i].name == NULL)
    return &aliases[i];

  if (alias_rehash ())
    return NULL;

  return alias_lookup_or_install (name, install);
}

static int
alias_lookup (char *name, mu_list_t *plist)
{
  alias_t *ap = alias_lookup_or_install (name, 0);
  if (ap)
    {
      *plist = ap->list;
      return 1;
    }
  return 0;
}

void
alias_print (char *name)
{
  if (!name)
    {
      unsigned int i;

      if (!aliases)
	return;

      for (i = 0; i < hash_size[hash_num]; i++)
	{
	  if (aliases[i].name)
	    alias_print_group (aliases[i].name, aliases[i].list);
	}
    }
  else
    {
      mu_list_t list;

      if (!alias_lookup (name, &list))
	{
	  util_error (_("\"%s\": not a group"), name);
	  return;
	}
      alias_print_group (name, list);
    }
}

int
alias_create (char *name, mu_list_t *plist)
{
  alias_t *ap = alias_lookup_or_install (name, 1);
  if (!ap)
    return 1;

  if (!ap->name)
    {
      /* new entry */
      if (mu_list_create (&ap->list))
	return 1;
      ap->name = strdup (name);
      if (!ap->name)
	return 1;
    }

  *plist = ap->list;

  return 0;
}

void
alias_print_group (char *name, mu_list_t list)
{
  fprintf (ofile, "%s    ", name);
  util_slist_print (list, 0);
  fprintf (ofile, "\n");
}

void
alias_destroy (char *name)
{
  unsigned int i, j, r;
  alias_t *alias = alias_lookup_or_install (name, 0);
  if (!alias)
    return;
  free (alias->name);
  util_slist_destroy (&alias->list);

  for (i = alias - aliases;;)
    {
      aliases[i].name = NULL;
      j = i;

      do
	{
	  if (++i >= hash_size[hash_num])
	    i = 0;
	  if (!aliases[i].name)
	    return;
	  r = hash(aliases[i].name);
	}
      while ((j < r && r <= i) || (i < j && j < r) || (r <= i && i < j));

      aliases[j] = aliases[i];
    }
}

static void
recursive_alias_expand (char *name, mu_list_t exlist, mu_list_t origlist)
{ 
  mu_list_t alist;
  mu_iterator_t itr;

  if (!alias_lookup (name, &alist))
    {
      if (mu_list_locate (exlist, name, NULL) == MU_ERR_NOENT)
	mu_list_append (exlist, name);
      return;
    }
  
  mu_list_get_iterator (alist, &itr);
  for (mu_iterator_first (itr);
       !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      char *word;
      
      mu_iterator_current (itr, (void **)&word);
      if (mu_list_locate (origlist, word, NULL) == MU_ERR_NOENT)
	{
	  mu_list_prepend (origlist, word);
	  recursive_alias_expand (word, exlist, origlist);
	  mu_list_remove (origlist, word);
	}
    }
  mu_iterator_destroy (&itr);
}

static int
string_comp (const void *item, const void *value)
{
  return strcmp (item, value);
}

char *
alias_expand (char *name)
{
  mu_list_t list;

  if (util_getenv (NULL, "recursivealiases", Mail_env_boolean, 0) == 0)
    {
      char *s;
      mu_list_t origlist;
      
      int status = mu_list_create (&list);
      if (status)
	{
	  mu_error (_("Cannot create list: %s"), mu_strerror (status));
	  return NULL;
	}
      status = mu_list_create (&origlist);
      if (status)
	{
	  mu_list_destroy (&origlist);
	  mu_error (_("Cannot create list: %s"), mu_strerror (status));
	  return NULL;
	}
      mu_list_set_comparator (list, string_comp);
      mu_list_set_comparator (origlist, string_comp);
      recursive_alias_expand (name, list, origlist);
      s = util_slist_to_string (list, ",");
      mu_list_destroy (&origlist);
      mu_list_destroy (&list);
      return s;
    }
  
  if (!alias_lookup (name, &list))
    return NULL;
  return util_slist_to_string (list, ",");
}

struct alias_iterator
{
  const char *prefix;
  int prefixlen;
  int pos;
};

const char *
alias_iterate_next (alias_iterator_t itr)
{
  int i;
  for (i = itr->pos; i < hash_size[hash_num]; i++)
    if (aliases[i].name
	&& strlen (aliases[i].name) >= itr->prefixlen
	&& strncmp (aliases[i].name, itr->prefix, itr->prefixlen) == 0)
      {
	itr->pos = i + 1;
	return aliases[i].name;
      }
  return NULL;
}

const char *
alias_iterate_first (const char *prefix, alias_iterator_t *pc)
{
  struct alias_iterator *itr;

  if (!aliases)
    {
      *pc = NULL;
      return NULL;
    }
  
  itr = xmalloc (sizeof *itr);
  itr->prefix = prefix;
  itr->prefixlen = strlen (prefix);
  itr->pos = 0;
  *pc = itr;
  return alias_iterate_next (itr);
}

void
alias_iterate_end (alias_iterator_t *pc)
{
  free (*pc);
  *pc = NULL;
}

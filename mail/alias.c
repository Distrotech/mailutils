/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

static void alias_print __P ((char *name));
static void alias_print_group __P ((char *name, list_t list));
static int  alias_create __P ((char *name, list_t *plist));
static int  alias_lookup __P ((char *name, list_t *plist));

/*
 * a[lias] [alias [address...]]
 * g[roup] [alias [address...]]
 */

int
mail_alias (int argc, char **argv)
{
  if (argc == 1)
      alias_print(NULL);
  else if (argc == 2)
      alias_print(argv[1]);
  else
    {
      list_t list;

      if (alias_create(argv[1], &list))
	return 1;

      argc --;
      argv ++;
      while (--argc)
	util_slist_add(&list, *++argv);
    }
  return 0;
}

typedef struct _alias alias_t;

struct _alias
{
  char *name;
  list_t list;
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

static unsigned int hash __P((char *name));
static int alias_rehash __P((void));
static alias_t *alias_lookup_or_install __P((char *name, int install));
static void alias_print_group __P((char *name, list_t list));

unsigned
hash(char *name)
{
    unsigned i;

    for (i = 0; *name; name++) {
        i <<= 1;
        i ^= *(unsigned char*) name;
    }
    return i % hash_size[hash_num];
}

int
alias_rehash()
{
  alias_t *old_aliases = aliases;
  alias_t *ap;
  unsigned int i;

  if (++hash_num >= max_rehash)
    {
      util_error("alias hash table full");
      return 1;
    }

  aliases = xcalloc(hash_size[hash_num], sizeof (aliases[0]));
  if (old_aliases)
    {
      for (i = 0; i < hash_size[hash_num-1]; i++)
	{
	  if (old_aliases[i].name)
	    {
	      ap = alias_lookup_or_install(old_aliases[i].name, 1);
	      ap->name = old_aliases[i].name;
	      ap->list = old_aliases[i].list;
	    }
	}
      free (old_aliases);
    }
  return 0;
}

alias_t *
alias_lookup_or_install(char *name, int install)
{
  unsigned i, pos;

  if (!aliases)
    {
      if (install)
	{
	  if (alias_rehash())
	    return NULL;
	}
      else
	return NULL;
    }

  pos = hash(name);

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

  if (alias_rehash())
    return NULL;

  return alias_lookup_or_install(name, install);
}

static int
alias_lookup(char *name, list_t *plist)
{
  alias_t *ap = alias_lookup_or_install(name, 0);
  if (ap)
    {
      *plist = ap->list;
      return 1;
    }
  return 0;
}

void
alias_print(char *name)
{
  if (!name)
    {
      unsigned int i;

      if (!aliases)
	return;

      for (i = 0; i < hash_size[hash_num]; i++)
	{
	  if (aliases[i].name)
	    alias_print_group(aliases[i].name, aliases[i].list);
	}
    }
  else
    {
      list_t list;

      if (!alias_lookup(name, &list))
	{
	  util_error(_("\"%s\": not a group"), name);
	  return;
	}
      alias_print_group(name, list);
    }
}

int
alias_create(char *name, list_t *plist)
{
  alias_t *ap = alias_lookup_or_install(name, 1);
  if (!ap)
    return 1;

  if (!ap->name)
    {
      /* new entry */
      if (list_create(&ap->list))
	return 1;
      ap->name = strdup(name);
      if (!ap->name)
	return 1;
    }

  *plist = ap->list;

  return 0;
}

void
alias_print_group(char *name, list_t list)
{
  fprintf(ofile, "%s    ", name);
  util_slist_print(list, 0);
  fprintf(ofile, "\n");
}

void
alias_destroy(char *name)
{
  unsigned int i, j, r;
  alias_t *alias = alias_lookup_or_install(name, 0);
  if (!alias)
    return;
  free(alias->name);
  util_slist_destroy(&alias->list);

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
	} while ((j < r && r <= i) || (i < j && j < r) || (r <= i && i < j));

      aliases[j] = aliases[i];
    }
}

char *
alias_expand(char *name)
{
  list_t list;

  if (!alias_lookup(name, &list))
    return NULL;
  return util_slist_to_string(list, ",");
}

/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include <address0.h>
#include <misc.h>

/*
 * parseaddr.c	Read a valid RFC822 address with all the comments
 *		etc in it, and return _just_ the email address.
 *
 * Version:	@(#)parseaddr.c  1.00  02-Apr-1999  miquels@cistron.nl
 *
 */

struct token
{
  struct token *next;
  char word[1];
};

#define SKIPSPACE(p) do { while(*p && isspace(*p)) p++; } while(0)

/* Skip everything between quotes.  */
static void
quotes (const char **ptr)
{
  const char *p = *ptr;

  p++;
  while (*p && *p != '"')
    {
      if (*p == '\\' && p[1])
	p++;
      p++;
    }
  *ptr = p;
}

/* Return the next token. A token can be "<>()," or any "word". */
static struct token *
gettoken (const char **ptr)
{
  struct token	*tok;
  const char *p = *ptr;
  const char *begin;
  int l, quit = 0;

  SKIPSPACE(p);
  begin = p;

  while (!quit)
    {
      switch (*p)
	{
	case 0:
	case ' ':
	case '\t':
	case '\n':
	  quit = 1;
	  break;
	case '(':
	case ')':
	case '<':
	case '>':
	case ',':
	  if (p == begin)
	    p++;
	  quit = 1;
	  break;
	case '\\':
	  if (p[1])
	    p++;
	  break;
	case '"':
	  quotes (&p);
	  break;
	}
      if (!quit)
	p++;
    }

  l = p - begin;
  if (l == 0)
    return NULL;
  tok = malloc (sizeof (struct token) + l);
  if (tok == NULL)
    return NULL;
  tok->next = NULL;
  strncpy (tok->word, begin, l);
  tok->word[l] = 0;

  SKIPSPACE (p);
  *ptr = p;

  return tok;
}

/* Get email address from rfc822 address.  */
/* Note: This again as for header.c an awfull way of doing things.
   Meaning I need a correct rfc822 Parser.  This one does not
   understand group.  There is not doubt a better way to do this.  */
int
address_create (address_t *paddress, const char *addr)
{
  const char *p;
  struct token	*t, *tok, *last;
  struct token	*brace = NULL;
  struct token	*comments = NULL;
  struct token	*start_comments = NULL;
  address_t address;
  address_t head;
  int in_comment = 0;
  int status = 0;

  tok = last = NULL;

  address = calloc (1, sizeof (*address));
  if (address == NULL)
    return  ENOMEM;
  head = address;
  /* Read address, remove comments right away.  */
  p = addr;
  while ((t = gettoken(&p)) != NULL && (t->word[0] != ',' || in_comment))
    {
      if (t->word[0] == '(' || t->word[0] == ')' || in_comment)
	{
	  if (t->word[0] == '(')
	    in_comment++;
	  if (t->word[0] == ')')
	    in_comment--;
	  if (!start_comments)
	    comments = start_comments = t;
	  else
	    {
	      comments->next = t;
	      comments = t;
	    }
	  continue;
	}

      if (t->word[0] == '<')
	brace = t;

      if (tok)
	last->next = t;
      else
	tok = t;
      last = t;
    }

  address->addr = strdup (addr);
  if (address->addr == NULL)
    {
      address_destroy (&address);
      status = ENOMEM;
      goto freenodes;
    }

  /* Put extracted address into email  */
  t = brace ? brace->next : tok;
  for (; t && t->word[0] != ',' && t->word[0] != '>'; t = t->next)
    {
      char *tmp;
      if (address->email
	  && (tmp = realloc (address->email, strlen (address->email)
			     + strlen (t->word) + 1)) != NULL)
	{
	  address->email = tmp;
	  strcat (address->email, t->word);
	}
      else if  ((tmp = calloc (strlen (t->word) + 1, sizeof (char))) != NULL)
	{
	  address->email = tmp;
	  strcat (address->email, t->word);
	}
      else
	{
	  address_destroy (&address);
	  status = ENOMEM;
	  goto freenodes;
	}
    }

  /* Extract Comments.  */
  for (t = start_comments; t ; t = t->next)
    {
      char *tmp;
      if (t->word[0] == '(' || t->word[0] == ')')
	continue;
      if (address->comments
	  && (tmp = realloc (address->comments, strlen (address->comments)
			     + strlen (t->word) + 2)) != NULL)
	{
	  address->comments = tmp;
	  strcat (address->comments, " ");
	  strcat (address->comments, t->word);
	}
      else if  ((tmp = calloc (strlen (t->word) + 1, sizeof (char))) != NULL)
	{
	  address->comments = tmp;
	  strcat (address->comments, t->word);
	}
      else
	{
	  address_destroy (&address);
	  status = ENOMEM;
	  goto freenodes;
	}
    }

  /* Extract Personal.  */
  if (brace == NULL)
    {
      address->personal = strdup ("");
    }
  else
    {
      int in_brace = 0;
      for (t = tok; t ; t = t->next)
	{
	  char *tmp;
	  if (t->word[0] == '<' || t->word[0] == '>' || in_brace)
	    {
	      if (t->word[0] == '<')
		in_brace++;
	      else if (t->word[0] == '>')
		in_brace--;
	      continue;
	    }
	  if (address->personal
	      && (tmp = realloc (address->personal, strlen (address->personal)
			     + strlen (t->word) + 2)) != NULL)
	    {
	      address->personal = tmp;
	      strcat (address->personal, " ");
	      strcat (address->personal, t->word);
	    }
	  else if  ((tmp = calloc (strlen (t->word) + 1, sizeof (char))) != NULL)
	    {
	      address->personal = tmp;
	      strcat (address->personal, t->word);
	    }
	  else
	    {
	      address_destroy (&address);
	      status = ENOMEM;
	      goto freenodes;
	    }
	}
    }

  *paddress = head;

  /* Free list of tokens.  */
  freenodes:
  for (t = tok; t; t = last)
    {
      last = t->next;
      free (t);
    }

  return status;
}

void
address_destroy (address_t *paddress)
{
  if (paddress && *paddress)
    {
      address_t address = *paddress;
      address_t current;
      for (; address; address = current)
	{
	  if (address->comments)
	    free (address->comments);
	  if (address->personal)
	    free (address->personal);
	  if (address->email)
	    free (address->email);
	  current = address->next;
	  free (address);
	}
      *paddress = NULL;
    }
}

int
address_get_personal (address_t addr, char *buf, size_t len, size_t *n)
{
  size_t i;
  if (addr == NULL)
    return EINVAL;
  i = _cpystr (buf, addr->personal, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_comments (address_t addr, char *buf, size_t len, size_t *n)
{
  size_t i;
  if (addr == NULL)
    return EINVAL;
  i = _cpystr (buf, addr->comments, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_email (address_t addr, char *buf, size_t len, size_t *n)
{
  size_t i;
  if (addr == NULL)
    return EINVAL;
  i = _cpystr (buf, addr->email, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_address (address_t addr, char *buf, size_t len, size_t *n)
{
  size_t i;
  if (addr == NULL)
    return EINVAL;
  i = _cpystr (buf, addr->addr, len);
  if (n)
    *n = i;
  return 0;
}

#if 0
int
main (int argc, char **argv)
{
  address_t addr;
  int i;

  address_create (&addr, argv[1]);
  for (i = 0; addr; addr = addr->next, i++)
    {
      printf ("%d\n", i);
      if (addr->addr)
	printf ("Address |%s|\n", addr->addr);
      if (addr->comments)
	printf ("Comments |%s|\n", addr->comments);
      if (addr->personal)
	printf ("Personal |%s|\n", addr->personal);
      if (addr->email)
	printf ("Email |%s|\n", addr->email);
    }
  return 0;
}
#endif

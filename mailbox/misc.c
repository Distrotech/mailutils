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

#include <misc.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

size_t
_cpystr (char *dst, const char *src, size_t size)
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

/* Skip everything between quotes. */
static void
quotes (char **ptr)
{
  char *p = *ptr;

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
gettoken (char **ptr)
{
  struct token	*tok;
  char *p = *ptr;
  char *begin;
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
int
parseaddr (const char *addr, char *buf, size_t bufsz)
{
  const char *p;
  struct token	*t, *tok, *last;
  struct token	*brace = NULL;
  int comment = 0;

  tok = last = NULL;

  /* Read address, remove comments right away.  */
  p = addr;
  while ((t = gettoken(&p)) != NULL && t->word[0] != ',')
    {
      if (t->word[0] == '(' || t->word[0] == ')' || comment)
	{
	  free (t);
	  if (t->word[0] == '(')
	    comment++;
	  if (t->word[0] == ')')
	    comment--;
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

  /* Put extracted address into "buf"  */
  buf[0] = 0;
  t = brace ? brace->next : tok;
  for (; t && t->word[0] != ',' && t->word[0] != '>'; t = t->next)
    {
      if (strlen (t->word) >= bufsz)
	return -1;
      bufsz -= strlen (t->word);
      strcat (buf, t->word);
    }

  /* Free list of tokens.  */
  for (t = tok; t; t = last)
    {
      last = t->next;
      free (t);
    }

  return 0;
}

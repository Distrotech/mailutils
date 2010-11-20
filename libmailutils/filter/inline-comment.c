/* Simple inline comment filter for GNU Mailutils.
   Copyright (C) 2010 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

/* This filter removes all inline comments from the input.
   An inline comment begins with a comment starter character at the beginning
   of a line and ends with the newline character.

   Default comment starter is semicolon.  Another comment starter can
   be given as the first argument to the filter creation routine.

   Limitations:
    1. Comment starter must be a single character.  This should be
       fixed in future.
    2. Comment starter must appear at the beginning of a line.  This is
       hard (if not downright impossible) to fix, because the filter
       has no information about lexical structure of the input stream.
*/       

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/cctype.h>

enum ilcmt_state
  {
    ilcmt_initial,
    ilcmt_newline,
    ilcmt_copy,
    ilcmt_comment,
    ilcmt_ws,
    ilcmt_rollback
  };

#define ILCMT_REMOVE_EMPTY_LINES 0x01
#define ILCMT_SQUEEZE_WS         0x02

struct ilcmt_data
{
  enum ilcmt_state state;
  int cstart;
  int flags;
  char *buf;
  size_t size;
  size_t level;
  size_t replay;
};

enum ilcmt_action
  {
    action_echo,
    action_noecho,
    action_error
  };

#define ILCMT_BUF_INIT 80
#define ILCMT_BUF_INCR 16

static int
ilcmt_save (struct ilcmt_data *pd, int c)
{
  if (pd->level == pd->size)
    {
      size_t nsz;
      char *np;
      
      if (pd->size == 0)
	nsz = ILCMT_BUF_INIT;
      else
	nsz = pd->size + ILCMT_BUF_INCR;
      np = realloc (pd->buf, nsz);
      if (!np)
	return 1;
      pd->buf = np;
      pd->size = nsz;
    }
  pd->buf[pd->level++] = c;
  return 0;
}

static enum ilcmt_action
new_ilcmt_state (struct ilcmt_data *pd, int c)
{
  switch (pd->state)
    {
    case ilcmt_initial:
    case ilcmt_newline:
      if (c == pd->cstart)
	{
	  pd->state = ilcmt_comment;
	  return action_noecho;
	}
      else if (c == '\n')
	{
	  if (pd->flags & ILCMT_REMOVE_EMPTY_LINES)
	    return action_noecho;
	}
      else if (mu_isspace (c))
	{
	  if (pd->flags & ILCMT_REMOVE_EMPTY_LINES)
	    {
	      pd->state = ilcmt_ws;
	      pd->level = 0;
	      if (!(pd->flags & ILCMT_SQUEEZE_WS))
		{
		  if (ilcmt_save (pd, c))
		    return action_error;
		}
	      return action_noecho;
	    }
	}
      pd->state = ilcmt_copy;
      break;

    case ilcmt_ws:
      if (c == '\n')
	pd->state = ilcmt_newline;
      else if (mu_isspace (c))
	{
	  if (!(pd->flags & ILCMT_SQUEEZE_WS))
	    {
	      if (ilcmt_save (pd, c))
		return action_error;
	    }
	}
      else
	{
	  pd->replay = 0;
	  pd->state = ilcmt_rollback;
	}
      return action_noecho;
      
    case ilcmt_copy:
      if (c == '\n')
	pd->state = ilcmt_newline;
      break;

    case ilcmt_comment:
      if (c == '\n')
	pd->state = ilcmt_newline;
      return action_noecho;

    default:
      /* should not happen */
      break;
    }
  return action_echo;
}

static enum mu_filter_result
_ilcmt_decoder (void *xd, enum mu_filter_command cmd,
		struct mu_filter_io *iobuf)
{
  struct ilcmt_data *pd = xd;
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
      pd->state = ilcmt_initial;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  i = j = 0;
  if (pd->state == ilcmt_rollback)
    {
      if (pd->flags & ILCMT_SQUEEZE_WS)
	{
	  if (j == osize)
	    {
	      iobuf->osize = 1;
	      return mu_filter_moreoutput;
	    }
	  optr[j++] = ' ';
	  pd->state = ilcmt_copy;
	}
      else
	while (j < osize)
	  {
	    if (pd->replay == pd->level)
	      {
		pd->state = ilcmt_copy;
		break;
	      }
	    optr[j++] = pd->buf[pd->replay++];
	  }
      
      if (pd->state == ilcmt_copy)
	{
	  /* Clear the buffer state. */
	  pd->level = pd->replay = 0;
	}
    }
    
  for (; i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      enum ilcmt_action action = new_ilcmt_state (pd, c);
      if (action == action_echo)
	optr[j++] = c;
      else if (action == action_noecho)
	{
	  if (pd->state == ilcmt_rollback)
	    break;
	}
      else
	return mu_filter_failure;
    }

  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  struct ilcmt_data *pd = malloc (sizeof (*pd));
  int i;
  
  if (!pd)
    return ENOMEM;

  pd->cstart = ';';
  pd->flags = 0;
  pd->buf = NULL;
  pd->size = pd->level = pd->replay = 0;

  for (i = 1; i < argc; i++)
    {
      if (argv[i][1] == 0)
	pd->cstart = argv[i][0];
      else if (argv[i][0] == '-')
	{
	  switch (argv[i][1])
	    {
	    case 'r':
	      pd->flags |= ILCMT_REMOVE_EMPTY_LINES;
	      break;

	    case 's':
	      pd->flags |= ILCMT_SQUEEZE_WS;
	      break;

	    default:
	      free (pd);
	      return MU_ERR_PARSE;
	    }
	}
    }

  *pret = pd;
  
  return 0;
}

static struct _mu_filter_record _inline_comment_filter = {
  "INLINE-COMMENT",
  0,
  alloc_state,
  NULL,
  _ilcmt_decoder
};

mu_filter_record_t mu_inline_comment_filter = &_inline_comment_filter;




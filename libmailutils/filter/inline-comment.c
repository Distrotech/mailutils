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

enum ilcmt_state
  {
    ilcmt_initial,
    ilcmt_newline,
    ilcmt_copy,
    ilcmt_comment
  };

struct ilcmt_data
{
  enum ilcmt_state state;
  int cstart;
};

static int
new_ilcmt_state (struct ilcmt_data *pd, int c)
{
  switch (pd->state)
    {
    case ilcmt_initial:
    case ilcmt_newline:
      if (c == pd->cstart)
	{
	  pd->state = ilcmt_comment;
	  return 0;
	}
      else
	pd->state = ilcmt_copy;
      break;
      
    case ilcmt_copy:
      if (c == '\n')
	pd->state = ilcmt_newline;
      break;

    case ilcmt_comment:
      if (c == '\n')
	pd->state = ilcmt_newline;
      return 0;
    }
  return 1;
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

  for (i = j = 0; i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      if (new_ilcmt_state (pd, c))
	optr[j++] = c;
    }

  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  struct ilcmt_data *pd = malloc (sizeof (*pd));
  
  if (!pd)
    return ENOMEM;

  if (argc == 2)
    pd->cstart = argv[1][0];
  else
    pd->cstart = ';';
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




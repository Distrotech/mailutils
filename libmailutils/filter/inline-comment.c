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

/* In decode mode, this filter removes all inline comments from the input.
   An inline comment begins with a comment character sequence at the beginning
   of a line and ends with the next newline character.

   Default comment sequence is semicolon.  Another sequence can
   be given as the first argument to the filter creation routine.

   The following options modify this behavior:

      -r   Remove empty lines, i.e. the ones that contain only whitespace
           characters.
      -s   Squeeze whitespace.  Each sequence of two or more whitespace
           characters encountered on input is replaced by a single space
	   character on output.
      -S   A "following whitespace mode".  A comment sequence is recognized
           only if followed by a whitespace character.  The character itself
	   is retained on output.  

   In encode mode, this filter adds a comment sequence at the beginning
   of each line.

   The following options apply to this mode:
   
      -S   A "following whitespace mode".  A single space character is
           output after each inserted comment sequence.
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
    ilcmt_partial,
    ilcmt_comment_ws,
    ilcmt_ws,
    ilcmt_rollback_ws,
    ilcmt_rollback_com
  };

#define ILCMT_REMOVE_EMPTY_LINES 0x01 /* Just that :) */
#define ILCMT_SQUEEZE_WS         0x02 /* Replace a series of whitespace
					 characters with a single space */
#define ILCMT_FOLLOW_WS          0x04 /* In decode mode: comment sequence is
					 recognized only if followed by a
					 whitespace character.
					 In encode mode: output a space after
					 each comment prefix. */

struct ilcmt_data
{
  enum ilcmt_state state;  /* Current state */
  char *comment;           /* Comment sequence ... */ 
  size_t length;           /* and its length */
  int flags;               /* ILCMT_ flags */
  char *buf;               /* Rollback buffer ... */
  size_t size;             /* and its size */
  size_t level;            /* In ilcmt_partial and ilcmt_rollback_com
			      states: number of bytes matched in the comment
			      sequence.
			      In ilcmt_initial, ilcmt_newline, and
			      ilcmt_rollback_ws states: number of characters
			      stored in buf. */
  size_t replay;           /* Index of the next-to-be-replayed character
			      in buf (if state==ilcmt_rollback_ws) or
			      comment (if state==ilcmt_rollback_com) */
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

static enum mu_filter_result
_ilcmt_decoder (void *xd, enum mu_filter_command cmd,
		struct mu_filter_io *iobuf)
{
  struct ilcmt_data *pd = xd;
  const unsigned char *iptr, *iend;
  char *optr, *oend;

  switch (cmd)
    {
    case mu_filter_init:
      pd->state = ilcmt_initial;
      return mu_filter_ok;
      
    case mu_filter_done:
      free (pd->comment);
      free (pd->buf);
      return mu_filter_ok;
      
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  iend = iptr + iobuf->isize;
  optr = iobuf->output;
  oend = optr + iobuf->osize;

  while (iptr < iend && optr < oend)
    {
      switch (pd->state)
	{
	case ilcmt_initial:
	case ilcmt_newline:
	  if (*iptr == *pd->comment)
	    {
	      iptr++;
	      pd->level = 1;
	      pd->state = ilcmt_partial;
	    }
	  else if (*iptr == '\n')
	    {
	      if (pd->flags & ILCMT_REMOVE_EMPTY_LINES)
		{
		  iptr++;
		  continue;
		}
	      else
		*optr++ = *iptr++;
	    }
	  else if (mu_isspace (*iptr))
	    {
	      if (pd->flags & ILCMT_REMOVE_EMPTY_LINES)
		{
		  pd->state = ilcmt_ws;
		  pd->level = 0;
		  if (!(pd->flags & ILCMT_SQUEEZE_WS))
		    {
		      if (ilcmt_save (pd, *iptr))
			return mu_filter_failure;
		    }
		  iptr++;
		}
	      else
		{
		  *optr++ = *iptr++;
		  pd->state = ilcmt_copy;
		}
	    }
	  else
	    {
	      *optr++ = *iptr++;
	      pd->state = ilcmt_copy;
	    }
	  break;
	  
	case ilcmt_partial:
	  if (pd->level == pd->length)
	    {
	      if (pd->flags & ILCMT_FOLLOW_WS)
		pd->state = ilcmt_comment_ws;
	      else
		pd->state = ilcmt_comment;
	    }
	  else if (*iptr == pd->comment[pd->level])
	    {
	      iptr++;
	      pd->level++;
	    }
	  else
	    {
	      /* Begin restoring */
	      pd->replay = 0;
	      pd->state = ilcmt_rollback_com;
	    }
	  break;

	case ilcmt_comment_ws:
	  if (mu_isspace (*iptr))
	    {
	      iptr++;
	      pd->state = ilcmt_comment;
	    }
	  else
	    {
	      /* Begin restoring */
	      pd->replay = 0;
	      pd->state = ilcmt_rollback_com;
	    }
	  break;
	  
	case ilcmt_ws:
	  if (*iptr == '\n')
	    {
	      iptr++;
	      pd->state = ilcmt_newline;
	    }
	  else if (mu_isspace (*iptr))
	    {
	      if (!(pd->flags & ILCMT_SQUEEZE_WS))
		{
		  if (ilcmt_save (pd, *iptr))
		    return mu_filter_failure;
		}
	      iptr++;
	    }
	  else
	    {
	      pd->replay = 0;
	      pd->state = ilcmt_rollback_ws;
	    }
	  break;
      
	case ilcmt_copy:
	  if ((*optr++ = *iptr++) == '\n')
	    pd->state = ilcmt_newline;
	  break;

	case ilcmt_comment:
	  if (*iptr++ == '\n')
	    pd->state = ilcmt_newline;
	  break;

	case ilcmt_rollback_com:
	  *optr++ = pd->comment[pd->replay++];
	  if (pd->replay == pd->level)
	    pd->state = ilcmt_copy;
	  break;

	case ilcmt_rollback_ws:
	  if (pd->flags & ILCMT_SQUEEZE_WS)
	    {
	      *optr++ = ' ';
	      pd->state = ilcmt_copy;
	    }
	  else
	    {
	      *optr++ = pd->buf[pd->replay++];
	      if (pd->replay == pd->level)
		pd->state = ilcmt_copy;
	    }
	}
    }
  iobuf->isize = iptr - (const unsigned char *) iobuf->input;
  iobuf->osize = optr - iobuf->output;
  return mu_filter_ok;
}

static enum mu_filter_result
_ilcmt_encoder (void *xd,
		enum mu_filter_command cmd,
		struct mu_filter_io *iobuf)
{
  struct ilcmt_data *pd = xd;
  const unsigned char *iptr, *iend;
  char *optr, *oend;

  switch (cmd)
    {
    case mu_filter_init:
      pd->state = ilcmt_initial;
      return mu_filter_ok;
      
    case mu_filter_done:
      free (pd->comment);
      free (pd->buf);
      return mu_filter_ok;
      
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  iend = iptr + iobuf->isize;
  optr = iobuf->output;
  oend = optr + iobuf->osize;

  while (iptr < iend && optr < oend)
    {
      switch (pd->state)
	{
	case ilcmt_initial:
	case ilcmt_newline:
	  pd->replay = 0;
	  pd->state = ilcmt_rollback_com;
	case ilcmt_rollback_com:
	  *optr++ = pd->comment[pd->replay++];
	  if (pd->replay == pd->length)
	    pd->state = (pd->flags & ILCMT_FOLLOW_WS) ?
	                   ilcmt_ws : ilcmt_copy;
	  break;

	case ilcmt_ws:
	  *optr++ = ' ';
	  pd->state = ilcmt_copy;
	case ilcmt_copy:
	  if ((*optr++ = *iptr++) == '\n')
	    pd->state = ilcmt_newline;
	  break;

	default:
	  /* The rest of states is not used, so: */
	  abort ();
	}
    }
  iobuf->isize = iptr - (const unsigned char *) iobuf->input;
  iobuf->osize = optr - iobuf->output;
  return mu_filter_ok;
}
      

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  struct ilcmt_data *pd = malloc (sizeof (*pd));
  int i;
  const char *comment = ";";

  if (!pd)
    return ENOMEM;


  pd->flags = 0;
  pd->buf = NULL;
  pd->size = pd->level = pd->replay = 0;

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
	{
	  switch (argv[i][1])
	    {
	    case 'r':
	      pd->flags |= ILCMT_REMOVE_EMPTY_LINES;
	      break;

	    case 's':
	      pd->flags |= ILCMT_SQUEEZE_WS;
	      break;

	    case 'S':
	      pd->flags |= ILCMT_FOLLOW_WS;
	      break;
	       
	    default:
	      free (pd);
	      return MU_ERR_PARSE;
	    }
	}
      else
	comment = argv[i];
    }
  pd->comment = strdup (comment);
  if (!pd->comment)
    {
      free (pd);
      return ENOMEM;
    }
  pd->length = strlen (comment);
  
  *pret = pd;
  
  return 0;
}

static struct _mu_filter_record _inline_comment_filter = {
  "INLINE-COMMENT",
  0,
  alloc_state,
  _ilcmt_encoder,
  _ilcmt_decoder
};

mu_filter_record_t mu_inline_comment_filter = &_inline_comment_filter;




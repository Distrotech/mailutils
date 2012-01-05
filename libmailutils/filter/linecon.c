/* Simple inline comment filter for GNU Mailutils.
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

/* linecon - a line continuation filter.

   This filter has only decode mode.  It removes from the input
   backslashes immediately followed by a newline, thus implementing
   a familiar UNIX line-continuation facility.

   The following option is accepted:

      -i STR  Emit line number information after each contiguous sequence
              of joined lines.  STR supplies "information starter" -- a
	      sequence of characters which is output before the actual line
	      number.

*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/cctype.h>
#include <mailutils/io.h>

#define LINECON_LINE_INFO          0x01 /* Emit line number information */ 
#define LINECON_LINE_INFO_STATIC   0x02 /* Line info starter is in static
					   storage */
#define LINECON_CONTINUATION       0x04 /* Line continuation in progress */

enum linecon_state
  {
    linecon_init,                  /* Initial state */ 
    linecon_escape,                /* An escape was seen */
    linecon_rollback               /* Rollback in progress */
  };

struct linecon_data
{
  int flags;                 /* Mode flags */
  enum linecon_state state;  /* Current state */
  char *line_info_starter;   /* Starter for line number info lines */
  unsigned long line_number; /* Current line number */
  char sbuf[2];              /* Static buffer for single-character starters */
  char *buf;                 /* Rollback buffer ... */
  size_t size;               /* and its size */
  size_t level;              /* Number of characters stored in buf. */
  size_t replay;             /* Index of the next-to-be-replayed character
				from buf */
};

static enum mu_filter_result
_linecon_decoder (void *xd, enum mu_filter_command cmd,
		  struct mu_filter_io *iobuf)
{
  struct linecon_data *pd = xd;
  const unsigned char *iptr, *iend;
  char *optr, *oend;

  switch (cmd)
    {
    case mu_filter_init:
      pd->state = linecon_init;
      return mu_filter_ok;
      
    case mu_filter_done:
      if ((pd->flags & LINECON_LINE_INFO) &&
	  !(pd->flags & LINECON_LINE_INFO_STATIC))
	free (pd->line_info_starter);
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
	case linecon_init:
	  switch (*iptr)
	    {
	    case '\\':
	      iptr++;
	      pd->state = linecon_escape;
	      continue;
	    case '\n':
	      pd->line_number++;
	      if (pd->flags & LINECON_CONTINUATION)
		{
		  pd->flags &= ~LINECON_CONTINUATION;
		  mu_asnprintf (&pd->buf, &pd->size, "%s %lu\n",
				pd->line_info_starter,
				pd->line_number);
		  pd->level = strlen (pd->buf);
		  pd->replay = 0;
		  pd->state = linecon_rollback;
		}
	      /* fall through */
	    default:
	      *optr++ = *iptr++;
	      break;
	    }
	  break;

	case linecon_escape:
	  if (*iptr == '\n')
	    {
	      pd->line_number++;
	      iptr++;
	      pd->state = linecon_init;
	      if (pd->flags & LINECON_LINE_INFO)
		pd->flags |= LINECON_CONTINUATION;
	      continue;
	    }
	  else
	    {
	      pd->state = linecon_init;
	      *optr++ = '\\';
	      if (optr == oend)
		break;
	      *optr++ = *iptr++;
	    }
	  break;
	  
	case linecon_rollback:
	  *optr++ = pd->buf[pd->replay++];
	  if (pd->replay == pd->level)
	    pd->state = linecon_init;
	}
    }

  iobuf->isize = iptr - (const unsigned char *) iobuf->input;
  iobuf->osize = optr - iobuf->output;
  return mu_filter_ok;
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  int i;
  struct linecon_data *pd = malloc (sizeof (pd[0]));
  const char *line_info;
  
  if (!pd)
    return ENOMEM;
  pd->flags = 0;
  pd->buf = NULL;
  pd->size = pd->level = pd->replay = 0;
  pd->line_number = 1;
  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
	{
	  switch (argv[i][1])
	    {
	    case 'i':
	      pd->flags |= LINECON_LINE_INFO;
	      if (i + 1 == argc)
		return MU_ERR_PARSE;
	      line_info = argv[++i];
	      break;
	      
	    default:
	      free (pd);
	      return MU_ERR_PARSE;
	    }
	}
    }
  if (pd->flags & LINECON_LINE_INFO)
    {
      if (line_info[1])
	{
	  pd->line_info_starter = strdup (line_info);
	  if (!pd->line_info_starter)
	    {
	      free (pd);
	      return ENOMEM;
	    }
	}
      else
	{
	  pd->sbuf[0] = line_info[0];
	  pd->sbuf[1] = 0;
	  pd->line_info_starter = pd->sbuf;
	  pd->flags |= LINECON_LINE_INFO_STATIC;
	}
    }
  *pret = pd;
  return 0;
}

static struct _mu_filter_record _linecon_filter = {
  "LINECON",
  alloc_state,
  NULL,
  _linecon_decoder,
};

mu_filter_record_t mu_linecon_filter = &_linecon_filter;

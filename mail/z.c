/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

/* Scroll directions */
#define D_BWD  -1 /* z- */
#define D_NONE 0  /* z. */
#define D_FWD  1  /* z+ */

/*
 * z [+|-|. [count]]
 * Optional [count] specifies number of pages to skip before
 * displaying from lines. Default is 1.
 * . modifier causes command to redisplay the current page, i.e.
 * starting from the current message.
 */

static int
z_parse_args(int argc, char **argv, int *return_count, int *return_dir)
{
  int count = 1;
  int mul = 1;
  int dir = D_FWD;
  int an = 0;
  char *argp = NULL;

  argp = &argv[an][1];
  if (*argp == 0)
    {
      an++;
      if (an < argc)
	argp = argv[an];
    }

  if (*argp)
    {
      switch (*argp++)
       {
       case '+':
	 break;
       case '-':
	 dir = D_BWD;
	 break;
       case '.':
	 dir = D_NONE;
	 break;
       default:
	 util_error("Bad arguments for the scrolling command");
	 return 1;
       }

      if (*argp == 0)
	{
	  an++;
	  if (an < argc)
	    argp = argv[an];
	}

      argc -= an;

      if (argc > 1)
	{
	  util_error("Too many arguments for the scrolling command");
	  return 1;
	}

      if (argp && *argp)
	{
	  if (dir == D_NONE)
	    {
	      util_error("argument not applicable for z.");
	      return 1;
	    }

	  if ((mul = atoi(argp)) == 0)
	    {
	      util_error("Bad number of pages");
	      return 1;
	    }
	}

   }

 *return_count = mul * count;
 *return_dir = dir;

 return 0;
}

int
mail_z (int argc, char **argv)
{
  unsigned int i, nlines;
  unsigned int pagelines = util_screen_lines();
  int count;
  int dir;

  if (z_parse_args(argc, argv, &count, &dir))
    return 1;

  nlines = pagelines;

  count *= pagelines;
  switch (dir)
    {
    case D_BWD:
      if (cursor < nlines)
	{
	  fprintf(stdout, "On first screenful of messages\n");
	  return 0;
	}
      if (cursor < count)
	cursor = 1;
      else
	cursor -= count;
      break;

    case D_FWD:
      if (cursor + pagelines > total)
	{
	  fprintf(stdout, "On last screenful of messages\n");
	  return 0;
	}

      cursor += count;

      if (cursor + nlines > total)
	nlines = total - cursor;

      if (nlines <= 0)
	{
	  fprintf(stdout, "On last screenful of messages\n");
	  return 0;
	}
      break;
      
    case D_NONE:
      {
	/* z. is a GNU extension, so it will be more useful
	   when we reach the last message to show a full screen
	   of the last message.  This behaviour is used on startup
	   when displaying the summary and the headers, new messages
	   are last but we want to display a screenful with the
	   real cursor set by summary() to the new message.  */
	/* Find the start of the last screen page.  */
	int lastpage =  total - pagelines + 1;
	if (lastpage <= 0)
	  lastpage = 1;
	if (cursor > lastpage)
	  {
	    realcursor = cursor;
	    cursor = lastpage;
	    for (i = 0; i < nlines; i++)
	      {
		mail_from(0, NULL);
		cursor++;
	      }
	    cursor = realcursor;
	    return 1;
	  }
      }
      break;
    }

  realcursor = cursor;

  for (i = 0; i < nlines; i++)
    {
      mail_from(0, NULL);
      cursor++;
    }

  cursor = realcursor;

  return 1;
}

/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Parse traditional MH options. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <mh_getopt.h>

int mh_optind = 1;
char *mh_optarg;
char *mh_optptr;

int
mh_getopt (int argc, char **argv, struct mh_option *mh_opt, const char *doc)
{
  struct mh_option *p;
  int optlen;
  
  if (mh_optind >= argc || argv[mh_optind] == NULL)
    return EOF;
  mh_optptr = argv[mh_optind];

  if (mh_optptr[0] == '+')
    {
      mh_optarg = mh_optptr + 1;
      mh_optind++;
      return '+';
    }
  
  if (mh_optptr[0] != '-')
    return EOF;

  optlen = strlen (mh_optptr+1);
  for (p = mh_opt; p->opt; p++)
    {
      if ((p->match_len <= optlen
	   && memcmp (mh_optptr+1, p->opt, optlen) == 0)
	  || (p->flags == MH_OPT_BOOL
	      && optlen > 2
	      && memcmp (mh_optptr+1, "no", 2) == 0
	      && strlen (p->opt) >= optlen-2
	      && memcmp (mh_optptr+3, p->opt, optlen-2) == 0))
	break;
    }
  
  if (p->opt)
    {
      switch (p->flags)
	{
	case MH_OPT_BOOL:
	  if (memcmp (mh_optptr+1, "no", 2) == 0)
	    mh_optarg = "no";
	  else
	    mh_optarg = "yes";
	  break;
	  
	case MH_OPT_ARG:
	  mh_optarg = argv[++mh_optind];
	  break;

	default:
	  mh_optarg = NULL;
	}
      mh_optind++;
      return p->key;
    }
  else if (!strcmp (mh_optptr+1, "help"))
    {
      mh_help (mh_opt, doc);
      exit (1);
    }
  return '?';
}

void
mh_help (struct mh_option *mh_opt, const char *doc)
{
  struct mh_option *p;

  printf ("Compatibility syntax:\n");
  printf ("%s [switches] %s\n", program_invocation_short_name, doc);
  printf ("  switches are:\n");
  
  for (p = mh_opt; p->opt; p++)
    {
      int len = strlen (p->opt);
      
      printf ("  -");
      if (p->flags == MH_OPT_BOOL)
	printf ("[no]");
      if (len > p->match_len)
	printf ("(%*.*s)%s",
		p->match_len, p->match_len, p->opt,
		p->opt + p->match_len);
      else
	printf ("%s", p->opt);
      
      if (p->flags == MH_OPT_ARG)
	printf (" %s", p->arg);
      printf ("\n");
    }
  printf ("  -help\n");
  printf ("\nPlease use GNU long options instead. Run %s --help for more info on these.\n", program_invocation_short_name);
}

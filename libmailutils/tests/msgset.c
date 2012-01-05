/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdlib.h>
#include <mailutils/mailutils.h>

static void
parse_msgrange (char *arg, struct mu_msgrange *range)
{
  size_t msgnum;
  char *p;
  
  errno = 0;
  msgnum = strtoul (arg, &p, 10);
  range->msg_beg = msgnum;
  if (*p == ':')
    {
      if (*++p == '*')
	msgnum = 0;
      else
	{
	  msgnum = strtoul (p, &p, 10);
	  if (*p)
	    {
	      mu_error ("error in message range near %s", p);
	      exit (1);
	    }
	}
    }
  else if (*p == '*')
    msgnum = 0;
  else if (*p)
    {
      mu_error ("error in message range near %s", p);
      exit (1);
    }

  range->msg_end = msgnum;
}

mu_msgset_t
parse_msgset (const char *arg)
{
  int rc;
  mu_msgset_t msgset;
  char *end;

  MU_ASSERT (mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM));
  if (arg)
    {
      rc = mu_msgset_parse_imap (msgset, MU_MSGSET_NUM, arg, &end);
      if (rc)
	{
	  mu_error ("mu_msgset_parse_imap: %s near %s",
		    mu_strerror (rc), end);
	  exit (1);
	}
    }
  return msgset;
}

int
main (int argc, char **argv)
{
  int i;
  char *msgset_string = NULL;
  mu_msgset_t msgset;
  
  mu_set_program_name (argv[0]);
  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];

      if (strcmp (arg, "-h") == 0 || strcmp (arg, "-help") == 0)
	{
	  mu_printf ("usage: %s [-msgset=SET] [-add=X[:Y]] [-del=X[:Y]] "
		     "[-addset=SET] [-delset=SET] ...\n",
		     mu_program_name);
	  return 0;
	}
      else if (strncmp (arg, "-msgset=", 8) == 0)
	msgset_string = arg + 8;
      else
	break;
    }

  msgset = parse_msgset (msgset_string);
  
  for (; i < argc; i++)
    {
      char *arg = argv[i];
      struct mu_msgrange range;
      
      if (strncmp (arg, "-add=", 5) == 0)
	{
	  parse_msgrange (arg + 5, &range);
	  MU_ASSERT (mu_msgset_add_range (msgset, range.msg_beg,
					  range.msg_end, MU_MSGSET_NUM));
	}
      else if (strncmp (arg, "-sub=", 5) == 0)
	{
	  parse_msgrange (arg + 5, &range);
	  MU_ASSERT (mu_msgset_sub_range (msgset, range.msg_beg,
					  range.msg_end, MU_MSGSET_NUM));
	}
      else if (strncmp (arg, "-addset=", 8) == 0)
	{
	  mu_msgset_t tset = parse_msgset (arg + 8);
	  if (!msgset)
	    msgset = tset;
	  else
	    {
	      MU_ASSERT (mu_msgset_add (msgset, tset));
	      mu_msgset_free (tset);
	    }
	}
      else if (strncmp (arg, "-subset=", 8) == 0)
	{
	  mu_msgset_t tset = parse_msgset (arg + 8);
	  if (!msgset)
	    {
	      mu_error ("no initial message set");
	      exit (1);
	    }
	  else
	    {
	      MU_ASSERT (mu_msgset_sub (msgset, tset));
	      mu_msgset_free (tset);
	    }
	}
      else
      	{
	  mu_error ("unknown option %s", arg);
	  return 1;
	}
    }
  MU_ASSERT (mu_msgset_print (mu_strout, msgset));
  mu_printf ("\n");
  mu_msgset_free (msgset);
  
  return 0;
}
  
	     

	

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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
parse_msgset (char *arg, struct mu_msgrange *range)
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

int
main (int argc, char **argv)
{
  int i;
  char *msgset_string = NULL;
  mu_msgset_t msgset;
  int rc;
  int flags = 0;
  mu_mailbox_t mbox = NULL;
  
  mu_set_program_name (argv[0]);
  mu_registrar_record (mu_mbox_record);
  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];

      if (strcmp (arg, "-h") == 0 || strcmp (arg, "-help") == 0)
	{
	  mu_printf ("usage: %s [-msgset=SET] [-add X[:Y]] [-del X:[Y]]...\n",
		     mu_program_name);
	  return 0;
	}
      else if (strncmp (arg, "-msgset=", 8) == 0)
	msgset_string = arg + 8;
      else if (strcmp (arg, "-uid") == 0)
	flags |= MU_MSGSET_UID;
      else if (strncmp (arg, "-mailbox=", 9) == 0)
	{
	  MU_ASSERT (mu_mailbox_create (&mbox, arg + 9));
	  MU_ASSERT (mu_mailbox_open (mbox, MU_STREAM_READ));
	}
      else
	break;
    }

  MU_ASSERT (mu_msgset_create (&msgset, mbox, flags));
  if (msgset_string)
    {
      char *end;
      rc = mu_msgset_parse_imap (msgset, msgset_string, &end);
      if (rc)
	{
	  mu_error ("mu_msgset_parse_imap: %s near %s",
		    mu_strerror (rc), end);
	  return 1;
	}
    }
  
  for (; i < argc; i++)
    {
      char *arg = argv[i];
      struct mu_msgrange range;
      
      if (strncmp (arg, "-add=", 5) == 0)
	{
	  parse_msgset (arg + 5, &range);
	  MU_ASSERT (mu_msgset_add_range (msgset, range.msg_beg,
					  range.msg_end));
	}
      else if (strncmp (arg, "-sub=", 5) == 0)
	{
	  parse_msgset (arg + 5, &range);
	  MU_ASSERT (mu_msgset_sub_range (msgset, range.msg_beg,
					  range.msg_end));
	}
      else if (strcmp (arg, "-neg") == 0)
	{
	  mu_msgset_t negated_set;
	  MU_ASSERT (mu_msgset_negate (msgset, &negated_set));
	  mu_msgset_free (msgset);
	  msgset = negated_set;
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
  
	     

	

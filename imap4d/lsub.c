/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2007-2012 Free Software Foundation, Inc.

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

#include "imap4d.h"
#include <mailutils/property.h>

/*
6.3.9.  LSUB Command

   Arguments:  reference name
               mailbox name with possible wildcards

   Responses:  untagged responses: LSUB

   Result:     OK - lsub completed
               NO - lsub failure: can't list that reference or name
               BAD - command unknown or arguments invalid
*/
int
imap4d_lsub (struct imap4d_session *session,
             struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *ref;
  char *wcard;
  char *pattern;
  mu_property_t prop;
  mu_iterator_t itr;
  
  if (imap4d_tokbuf_argc (tok) != 4)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");
  
  ref = imap4d_tokbuf_getarg (tok, IMAP4_ARG_1);
  wcard = imap4d_tokbuf_getarg (tok, IMAP4_ARG_2);

  pattern = mu_make_file_name (ref, wcard);
  if (!pattern)
    return io_completion_response (command, RESP_NO, "Not enough memory");

  prop = open_subscription ();
  if (!prop)
    return io_completion_response (command, RESP_NO, "Cannot unsubscribe");

  if ((rc = mu_property_get_iterator (prop, &itr)) == 0)
    {
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  const char *name, *val;
	  
	  mu_iterator_current_kv (itr, (const void **)&name, (void**)&val);

	  if (mu_imap_wildmatch (pattern, name, MU_HIERARCHY_DELIMITER) == 0)
	    io_untagged_response (RESP_NONE, "LSUB () \"%c\" \"%s\"",
				  MU_HIERARCHY_DELIMITER, name);
	}
    }
  else
    mu_diag_funcall (MU_DIAG_ERROR, "mu_property_get_iterator", NULL, rc);
  return io_completion_response (command, RESP_OK, "Completed");
}

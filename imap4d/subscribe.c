/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2007, 2009-2012 Free Software Foundation,
   Inc.

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

mu_property_t
open_subscription ()
{
  int rc;
  mu_property_t prop;
  mu_stream_t str;
  char *filename = mu_make_file_name (real_homedir, ".mu-subscr");
  
  rc = mu_file_stream_create (&str, filename, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_file_stream_create", filename, rc);
      return NULL;
    }
  rc = mu_property_create_init (&prop, mu_assoc_property_init, str);
  free (filename);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_create_init", NULL, rc);
      return NULL;
    }
  return prop;
}

/*
  6.3.6.  SUBSCRIBE Command
  
    Arguments:  mailbox

    Responses:  no specific responses for this command

    Result:     OK - subscribe completed
		NO - subscribe failure: can't subscribe to that name
		BAD - command unknown or arguments invalid
*/
int
imap4d_subscribe (struct imap4d_session *session,
                  struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *name;
  mu_property_t prop;
  
  if (imap4d_tokbuf_argc (tok) != 3)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");
  
  name = imap4d_tokbuf_getarg (tok, IMAP4_ARG_1);
  
  prop = open_subscription ();
  if (!prop)
    return io_completion_response (command, RESP_NO, "Cannot subscribe");
  rc = mu_property_set_value (prop, name, "", 1);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_property_set_value", name, rc);
  else
    {
      rc = mu_property_save (prop);
      if (rc)
	mu_diag_funcall (MU_DIAG_ERROR, "mu_property_save", NULL, rc);
    }  
  mu_property_destroy (&prop);
  
  if (rc)
    return io_completion_response (command, RESP_NO, "Cannot subscribe");
   
  return io_completion_response (command, RESP_OK, "Completed");
}

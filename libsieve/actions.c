/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve.h>

int
sieve_action_stop (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_log_action (mach, "STOP", NULL);
  mach->pc = 0;
  return 0;
}

int
sieve_action_keep (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_log_action (mach, "KEEP", NULL);
  return 0;
}

int
sieve_action_discard (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_log_action (mach, "DISCARD", "marking as deleted");
  if (sieve_is_dry_run (mach))
    return 0;
  sieve_mark_deleted (mach->msg, 1);
  return 0;
}

int
sieve_action_fileinto (sieve_machine_t mach, list_t args, list_t tags)
{
  int rc;
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, "fileinto: can't get filename!");
      sieve_abort (mach);
    }
  sieve_log_action (mach, "FILEINTO", "delivering into %s", val->v.string);
  if (sieve_is_dry_run (mach))
    return 0;

  rc = message_save_to_mailbox (mach->msg, mach->ticket, mach->mu_debug,
				val->v.string);
  if (rc)
    sieve_error (mach, "fileinto: cannot save to mailbox: %s",
		 mu_errstring (rc));
  else
    sieve_mark_deleted (mach->msg, 1);    
  
  return rc;
}

int
sieve_action_reject (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, "redirect: can't get text!");
      sieve_abort (mach);
    }
  sieve_log_action (mach, "REJECT", NULL);  
  if (sieve_is_dry_run (mach))
    return 0;
  return 0;
}

int
sieve_action_redirect (sieve_machine_t mach, list_t args, list_t tags)
{
  sieve_value_t *val = sieve_value_get (args, 0);
  if (!val)
    {
      sieve_error (mach, "redirect: can't get address!");
      sieve_abort (mach);
    }
  sieve_log_action (mach, "REDIRECT", "to %s", val->v.string);
  if (sieve_is_dry_run (mach))
    return 0;
  return 0;
}

sieve_data_type fileinto_args[] = {
  SVT_STRING,
  SVT_VOID
};

void
sieve_register_standard_actions ()
{
  sieve_register_action ("stop", sieve_action_stop, NULL, NULL, 1);
  sieve_register_action ("keep", sieve_action_keep, NULL, NULL, 1);
  sieve_register_action ("discard", sieve_action_discard, NULL, NULL, 1);
  sieve_register_action ("fileinto", sieve_action_fileinto, fileinto_args,
			 NULL, 1);
  sieve_register_action ("reject", sieve_action_reject, fileinto_args,
			 NULL, 0);
  sieve_register_action ("redirect", sieve_action_redirect, fileinto_args,
			 NULL, 0);
}


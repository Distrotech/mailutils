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
sieve_action_stop (sieve_machine_t *mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_action_keep (sieve_machine_t *mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_action_discard (sieve_machine_t *mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_action_fileinto (sieve_machine_t *mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_action_reject (sieve_machine_t *mach, list_t args, list_t tags)
{
  return 0;
}

int
sieve_action_redirect (sieve_machine_t *mach, list_t args, list_t tags)
{
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
  sieve_register_action ("discard", sieve_action_keep, NULL, NULL, 1);
  sieve_register_action ("fileinto", sieve_action_keep, fileinto_args,
			 NULL, 1);
  sieve_register_action ("reject", sieve_action_reject, fileinto_args,
			 NULL, 0);
  sieve_register_action ("redirect", sieve_action_redirect, fileinto_args,
			 NULL, 0);
}


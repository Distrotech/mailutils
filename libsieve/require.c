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

void
sieve_require (list_t slist)
{
  int status;
  iterator_t itr;
  
  status = iterator_create (&itr, slist);
  if (status)
    {
      sieve_compile_error (sieve_filename, sieve_line_num,
                   "cannot create iterator: %s",
		   mu_errstring (status));
      return;
    }

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *s;
      sieve_register_t *reg;
      
      iterator_current (itr, (void **)&s);

      /* FIXME: if (strncmp (s, "comparator-", 11) ... */
      reg = sieve_action_lookup (s);
      if (!reg)
	{
	  sieve_compile_error (sieve_filename, sieve_line_num,
                       "source for the required action %s is not available",
		       s);
	  break;
	}
      reg->required = 1;
    }
  iterator_destroy (&itr);
}
     

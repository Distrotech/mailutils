/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/sys/pop3.h>
#include <stdlib.h>

int
pop3_set_timeout (pop3_t pop3, int timeout)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  pop3->timeout = timeout;
  return 0;
}

int
pop3_get_timeout (pop3_t pop3, int *ptimeout)
{
  /* Sanity checks.  */
  if (pop3 == NULL || ptimeout == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  *ptimeout = pop3->timeout;
  return 0;
}

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

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <stdio.h>
#include <mailutils/sys/pop3.h>

int
pop3_get_debug (pop3_t pop3, mu_debug_t *pdebug)
{
  if (pop3 == NULL || pdebug == NULL)
    return MU_ERROR_INVALID_PARAMETER;


  if (pop3->debug == NULL)
    {
      stream_t stream;
      int status = stream_stdio_create (&stream, stderr);
      if (status == 0)
	status = mu_debug_stream_create (&pop3->debug, stream, 0);
      if (status != 0)
	return status;
    }
  mu_debug_ref (pop3->debug);
  *pdebug = pop3->debug;
  return 0;
}

int
pop3_set_debug (pop3_t pop3, mu_debug_t debug)
{
  if (pop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (pop3->debug)
    mu_debug_destroy (&pop3->debug);
  pop3->debug = debug;
  return 0;
}

int
pop3_debug_cmd (pop3_t pop3)
{
  if (pop3->debug)
    {
      mu_debug_print (pop3->debug, MU_DEBUG_PROT, pop3->io.buf);
    }
  return 0;
}

int
pop3_debug_ack (pop3_t pop3)
{
  if (pop3->debug)
    {
      mu_debug_print (pop3->debug, MU_DEBUG_PROT, pop3->ack.buf);
      mu_debug_print (pop3->debug, MU_DEBUG_PROT, "\n");
    }
  return 0;
}

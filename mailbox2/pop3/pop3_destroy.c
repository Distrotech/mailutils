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

#include <mailutils/error.h>
#include <mailutils/sys/pop3.h>
#include <stdlib.h>

void
pop3_destroy (pop3_t pop3)
{
  if (pop3)
    {
      if (pop3->ack.buf)
	free (pop3->ack.buf);

      if (pop3->io.buf)
	free (pop3->io.buf);

      if (pop3->timestamp)
	free (pop3->timestamp);

      monitor_destroy (pop3->lock);

      free (pop3);
    }
}

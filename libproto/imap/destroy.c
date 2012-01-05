/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>
#include <mailutils/list.h>

void
mu_imap_destroy (mu_imap_t *pimap)
{
  if (pimap && *pimap)
    {
      mu_imap_t imap = *pimap;
      
      if (imap->errstr)
	free (imap->errstr);

      if (imap->tag_str)
	free (imap->tag_str);
      if (imap->tag_buf)
	free (imap->tag_buf);
      
      mu_list_destroy (&imap->capa);
      
      mu_imapio_destroy (&imap->io);

      free (imap);

      *pimap = NULL;
    }
}

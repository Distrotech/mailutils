/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

/* Each message starts with a line that contaings three of four fieds:
   "From" SP evelope-sender SP date [SP moreinfo]
*/
int
mbox_get_separator (mbox_t mbox, unsigned int msgno, char **psep)
{

  mbox_debug_print (mbox, "get_separator(%u)", msgno);

  if (mbox == NULL || msgno  == 0 || psep == NULL)
      return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->messages_count)
    return MU_ERROR_INVALID_PARAMETER;

  msgno--;
  if (mbox->umessages[msgno]->separator)
    {
      *psep = strdup (mbox->umessages[msgno]->separator);
    }
  else
    {
      char *p = NULL;
      int len = 0 ;
      size_t n = 0;
      off_t offset = mbox->umessages[msgno]->from_;

      len = 2;
      p = malloc (len);
      if (p == NULL)
	return MU_ERROR_NO_MEMORY;
      *p = '\0';
      do
	{
	  char *s;
	  len += 128;
	  s = realloc (p, len);
	  if (s == NULL)
	    {
	      free (p);
	      return MU_ERROR_NO_MEMORY;
	    }
	  p = s ;
	  stream_readline (mbox->carrier, p + strlen (p), len, offset, &n);
	  offset += n;
	  n = strlen (p);
	} while (n && p[n - 1] != '\n');

      if (n && p[n - 1] == '\n')
	p[n - 1] = '\0';

      *psep = strdup (p);
      mbox->umessages[msgno]->separator = strdup (p);
      free (p);
    }
  return 0;
}

int
mbox_set_separator (mbox_t mbox, unsigned int msgno, const char *sep)
{
  if (mbox == NULL || msgno  == 0)
      return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->umessages_count)
    return MU_ERROR_INVALID_PARAMETER;

  msgno--;

  if (mbox->umessages[msgno]->separator)
    free (mbox->umessages[msgno]->separator);
  mbox->umessages[msgno]->separator = (sep) ? strdup (sep) : NULL;
  mbox->umessages[msgno]->attr_flags |= MU_ATTRIBUTE_MODIFIED;
  return 0;
}

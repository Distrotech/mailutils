/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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
#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/wordsplit.h>
#include <mailutils/cstr.h>

int
mu_string_split (const char *string, char *delim, mu_list_t list)
{
  size_t i;
  struct mu_wordsplit ws;
  int rc = 0;

  if (!string || !delim || !list)
    return EINVAL;

  /* Split the string */
  ws.ws_delim = delim;
  if (mu_wordsplit (string, &ws,
		    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|
		    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
    return errno;

  for (i = 0; i < ws.ws_wordc; i++)
    {
      rc = mu_list_append (list, ws.ws_wordv[i]);
      if (rc)
	break;
    }

  if (rc)
    {
      /* If failed, restore LIST to the state before entering this
	 function. */
      size_t j;
      mu_list_comparator_t cptr =
	mu_list_set_comparator (list, NULL);
      mu_list_destroy_item_t dptr =
	mu_list_set_destroy_item (list, NULL);

      for (j = 0; j < i; j++)
	mu_list_remove (list, ws.ws_wordv[j]);
      mu_list_set_destroy_item (list, dptr);
      mu_list_set_comparator (list, cptr);
    }
  else
    /* Make sure ws.ws_wordv[x] are not freed */
    ws.ws_wordc = 0;
  mu_wordsplit_free (&ws);
  return rc;
}

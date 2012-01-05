/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

#include <string.h>

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <mailutils/error.h>
#include <mailutils/list.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/pop3.h>

static int
capa_comp (const void *item, const void *value)
{
  const char *capa = item;
  const char *needle = value;
  for (; *needle; capa++, needle++)
    {
      if (!*capa)
	return 1;
      if (mu_tolower (*capa) != mu_tolower (*needle))
	return 1;
    }
  return !(*capa == 0 || mu_isspace (*capa));
}

/*
  CAPA command, return a list that contains the result.
  It is the responsability of the caller to destroy the list(mu_list_destroy).
 */
int
mu_pop3_capa (mu_pop3_t pop3, int reread, mu_iterator_t *piter)
{
  int status = 0;
  
  if (pop3 == NULL)
    return EINVAL;

  if (pop3->capa)
    {
      if (!reread)
	{
	  if (!piter)
	    return 0;
	  return mu_list_get_iterator (pop3->capa, piter);
	}
      mu_list_destroy (&pop3->capa);
    }

  status = mu_list_create (&pop3->capa);
  if (status)
    return status;
  mu_list_set_comparator (pop3->capa, capa_comp);
  mu_list_set_destroy_item (pop3->capa, mu_list_free_item);
  
  switch (pop3->state)
    {
    case MU_POP3_NO_STATE:
      status = mu_pop3_writeline (pop3, "CAPA\r\n");
      MU_POP3_CHECK_ERROR (pop3, status);
      MU_POP3_FCLR (pop3, MU_POP3_ACK);
      pop3->state = MU_POP3_CAPA;

    case MU_POP3_CAPA:
      status = mu_pop3_response (pop3, NULL);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      MU_POP3_CHECK_OK (pop3);
      pop3->state = MU_POP3_CAPA_RX;

    case MU_POP3_CAPA_RX:
      status = mu_pop3_read_list (pop3, pop3->capa);
      MU_POP3_CHECK_ERROR (pop3, status);
      if (piter)
	status = mu_list_get_iterator (pop3->capa, piter);
      else
	status = 0;
      pop3->state = MU_POP3_NO_STATE;
      break;

      /* They must deal with the error first by reopening.  */
    case MU_POP3_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }

  return status;
}

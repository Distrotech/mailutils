/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
# include <errno.h>
#include <stdlib.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_uidl_all (mu_pop3_t pop3, list_t *plist)
{
  int status;

  if (pop3 == NULL)
    return EINVAL;
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;

  switch (pop3->state)
    {
    case MU_POP3_NO_STATE:
      status = mu_pop3_writeline (pop3, "UIDL\r\n");
      MU_POP3_CHECK_ERROR (pop3, status);
      mu_pop3_debug_cmd (pop3);
      pop3->state = MU_POP3_UIDL;

    case MU_POP3_UIDL:
      status = mu_pop3_send (pop3);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      pop3->acknowledge = 0;
      pop3->state = MU_POP3_UIDL_ACK;

    case MU_POP3_UIDL_ACK:
      status = mu_pop3_response (pop3, NULL, 0, NULL);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      mu_pop3_debug_ack (pop3);
      MU_POP3_CHECK_OK (pop3);
      status = list_create (plist);
      MU_POP3_CHECK_ERROR(pop3, status);
      list_set_destroy_item(*plist, free);
      pop3->state = MU_POP3_UIDL_RX;

    case MU_POP3_UIDL_RX:
      {
        /* UIDL line are 512 octets maximum according to RFC 1939.
           But do not use the stack and malloc.  */
        char *uidla;
        size_t n = 0;
                                                                                                                             
        uidla = malloc (512);
        if (uidla == NULL)
          {
            MU_POP3_CHECK_ERROR(pop3, ENOMEM);
          }
        while ((status = mu_pop3_readline (pop3, uidla, 512, &n)) == 0 && n > 0)
          {
            /* Nuke the trailing newline  */
            if (uidla[n - 1] == '\n')
              uidla[n - 1] = '\0';
            /* add to the list.  */
            list_append (*plist, strdup (uidla));
            n = 0;
          }
        free (uidla);
        MU_POP3_CHECK_EAGAIN (pop3, status);
        pop3->state = MU_POP3_NO_STATE;
        break;
      }

      /* They must deal with the error first by reopening.  */
    case MU_POP3_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }

  return status;
}


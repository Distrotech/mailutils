/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

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

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <mailutils/error.h>
#include <mailutils/sys/nntp.h>

/*
  LIST EXTENSIONS command, return a list that contains the result.
  It is the responsability of the caller to destroy the list(list_destroy).
 */
int
mu_nntp_list_extensions (mu_nntp_t nntp, list_t *plist)
{
  int status;

  if (nntp == NULL)
    return EINVAL;
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;

  switch (nntp->state)
    {
    case MU_NNTP_NO_STATE:
      status = mu_nntp_writeline (nntp, "LIST EXTENSIONS\r\n");
      MU_NNTP_CHECK_ERROR (nntp, status);
      mu_nntp_debug_cmd (nntp);
      nntp->state = MU_NNTP_LIST_EXTENSIONS;

    case MU_NNTP_LIST_EXTENSIONS:
      status = mu_nntp_send (nntp);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->acknowledge = 0;
      nntp->state = MU_NNTP_LIST_EXTENSIONS_ACK;

    case MU_NNTP_LIST_EXTENSIONS_ACK:
      status = mu_nntp_response (nntp, NULL, 0, NULL);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      mu_nntp_debug_ack (nntp);
      MU_NNTP_CHECK_CODE (nntp, MU_NNTP_RESP_CODE_LIST_FOLLOW);
      status = list_create (plist);
      MU_NNTP_CHECK_ERROR(nntp, status);
      list_set_destroy_item(*plist, free);
      nntp->state = MU_NNTP_LIST_EXTENSIONS_RX;

    case MU_NNTP_LIST_EXTENSIONS_RX:
      {
        /* CAPA line are 512 octets maximum according to RFC 2449.
           But do not use the stack and malloc.  */
        char *capability;
        size_t n = 0;

        capability = malloc (512);
        if (capability == NULL)
          {
            /* MU_NNTP_CHECK_ERROR(nntp, ENOMEM);
	       We need to destroy the list if error.  */
	    nntp->io.ptr = nntp->io.buf;
	    nntp->state = MU_NNTP_ERROR;
	    list_destroy (*plist);
	    return ENOMEM;
          }
        while ((status = mu_nntp_readline (nntp, capability, 512, &n)) == 0 && n > 0)
          {
            /* Nuke the trailing newline  */
            if (capability[n - 1] == '\n')
              capability[n - 1] = '\0';
            /* add to the list.  */
            list_append (*plist, strdup (capability));
            n = 0;
          }
        free (capability);
        MU_NNTP_CHECK_EAGAIN (nntp, status);
        nntp->state = MU_NNTP_NO_STATE;
        break;
      }

      /* They must deal with the error first by reopening.  */
    case MU_NNTP_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }

  return status;
}

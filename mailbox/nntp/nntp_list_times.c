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
mu_nntp_list_active_times (mu_nntp_t nntp, const char *wildmat, list_t *plist)
{
  int status;

  if (nntp == NULL)
    return EINVAL;
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;

  switch (nntp->state)
    {
    case MU_NNTP_NO_STATE:
      if (wildmat == NULL || *wildmat == '\0')
	status = mu_nntp_writeline (nntp, "LIST ACTIVE.TIMES\r\n");
      else
	status = mu_nntp_writeline (nntp, "LIST ACTIVE.TIMES %s\r\n", wildmat);
      MU_NNTP_CHECK_ERROR (nntp, status);
      mu_nntp_debug_cmd (nntp);
      nntp->state = MU_NNTP_LIST_ACTIVE_TIMES;

    case MU_NNTP_LIST_ACTIVE_TIMES:
      status = mu_nntp_send (nntp);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->acknowledge = 0;
      nntp->state = MU_NNTP_LIST_ACTIVE_TIMES_ACK;

    case MU_NNTP_LIST_ACTIVE_TIMES_ACK:
      status = mu_nntp_response (nntp, NULL, 0, NULL);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      mu_nntp_debug_ack (nntp);
      MU_NNTP_CHECK_CODE (nntp, MU_NNTP_RESP_CODE_LIST_FOLLOW);
      status = list_create (plist);
      MU_NNTP_CHECK_ERROR(nntp, status);
      list_set_destroy_item(*plist, free);
      nntp->state = MU_NNTP_LIST_ACTIVE_TIMES_RX;

    case MU_NNTP_LIST_ACTIVE_TIMES_RX:
      {
        /* line are 512 octets maximum according to RFC.  */
        char *active;
        size_t n = 0;

        active = malloc (512);
        if (active == NULL)
          {
            /* MU_NNTP_CHECK_ERROR(nntp, ENOMEM);
	       We need to destroy the list if error.  */
	    nntp->io.ptr = nntp->io.buf;
	    nntp->state = MU_NNTP_ERROR;
	    list_destroy (*plist);
	    return ENOMEM;
          }
        while ((status = mu_nntp_readline (nntp, active, 512, &n)) == 0 && n > 0)
          {
            /* Nuke the trailing newline  */
            if (active[n - 1] == '\n')
              active[n - 1] = '\0';
            /* add to the list.  */
            list_append (*plist, strdup (active));
            n = 0;
          }
        free (active);
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

int
mu_nntp_parse_list_active_times (const char *buffer, char **group, unsigned long *time, char **creator)
{
  unsigned long dummy;
  char *name;
  char *owner;

  if (buffer == NULL || *buffer == '\0')
    return EINVAL;

  name = calloc(512, 1);
  if (name == NULL)
    return ENOMEM;

  owner = calloc(512, 1);
  if (owner == NULL)
    {
      free (name);
      return ENOMEM;
    }

  if (time == NULL)
    time = &dummy;

  sscanf (buffer, "%511s %ld %511s", name, time, owner);

  if (group == NULL)
    free (name);
  else
    *group = name;

  if (creator == NULL)
    free (owner);
  else
    *creator = owner;

  return 0;
}

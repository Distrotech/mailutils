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

#include <stdio.h>
#include <string.h>

#include <mailutils/md5-rsa.h>
#include <mailutils/sys/pop3.h>

/*
  APOP name digest
  a string identifying a mailbox and a MD5 digest string (both required)
*/
int
pop3_apop (pop3_t pop3, const char *user, const char *secret)
{
  int status;

  /* Sanity checks.  */
  if (pop3 == NULL || user == NULL || secret == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* The server did not offer a timestamp in the greeting, bailout early.  */
  if (pop3->timestamp == NULL)
    return MU_ERROR_NOT_SUPPORTED;

  switch (pop3->state)
    {
      /* Generate the md5 from the secret and timestamp.  */
    case POP3_NO_STATE:
      {
	MD5_CTX md5context;
	unsigned char md5digest[16];
	char digest[64]; /* Really it just has to be 32 + 1(null).  */
	char *tmp;
	size_t n;

	MD5Init (&md5context);
	MD5Update (&md5context, (unsigned char *)pop3->timestamp,
		   strlen (pop3->timestamp));
	MD5Update (&md5context, (unsigned char *)secret, strlen (secret));
	MD5Final (md5digest, &md5context);
	for (tmp = digest, n = 0; n < 16; n++, tmp += 2)
	  sprintf (tmp, "%02x", md5digest[n]);
	*tmp = '\0';

	status = pop3_writeline (pop3, "APOP %s %s\r\n", user, digest);
	POP3_CHECK_ERROR (pop3, status);
	pop3_debug_cmd (pop3);
	pop3->state = POP3_APOP;
      }

    case POP3_APOP:
      status = pop3_send (pop3);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3->acknowledge = 0;
      pop3->state = POP3_APOP_ACK;

    case POP3_APOP_ACK:
      status = pop3_response (pop3, NULL, 0, NULL);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3_debug_ack (pop3);
      POP3_CHECK_OK (pop3);
      pop3->state = POP3_NO_STATE;
      break;

      /* They must deal with the error first by reopening.  */
    case POP3_ERROR:
      status = MU_ERROR_OPERATION_CANCELED;
      break;

    default:
      status = MU_ERROR_OPERATION_IN_PROGRESS;
    }

  return status;
}

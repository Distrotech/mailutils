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

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <mailutils/sys/pop3.h>

static int pop3_sleep (int seconds);

/* Open the connection to the server.  */
int
pop3_open (pop3_t pop3, const char *host, unsigned int port, int flags)
{
  int status = 0;

  /* Sanity checks.  */
  if (pop3 == NULL || host == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* Default is 110.  */
  if (!port)
    port = 110;

  /* Enter the pop state machine, and boogy: AUTHORISATION State.  */
  switch (pop3->state)
    {
    default:
      /* __Fallthrough__, they want to clear an error.  */
      /* status = MU_ERROR_OPERATION_IN_PROGRESS; */
    case POP3_NO_STATE:
      /* Create the networking stack.  */
      if (pop3->stream == NULL)
	{
	  status = stream_tcp_create (&(pop3->stream));
	  POP3_CHECK_ERROR (pop3, status);
	  /* Using the awkward stream_t buffering.  */
	  /*stream_setbufsiz (pop3->stream, 1024); */
	}
      else
	{
          /* This is sudden death: for many pop servers, it is important to
             let them time to remove locks or move the .user.pop files.  This
             happen when we do close() and immediately open().  For example,
	     the user does not want to read the entire file, and wants start
	     to read a new message, closing the connection and immediately
	     contacting the server again, and he'll end up having
	     "-ERR Mail Lock busy" or something similar. To prevent this race
	     condition we sleep 2 seconds. */
	  stream_close (pop3->stream);
          pop3_sleep (2);
	}
      pop3->state = POP3_OPEN;

    case POP3_OPEN:
      /* Establish the connection.  */
      status = stream_open (pop3->stream, host, port, flags);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3->acknowledge = 0;
      pop3->state = POP3_GREETINGS;

    case POP3_GREETINGS:
      /* Get the greetings.  */
      {
	size_t len = 0;
	char *right, *left;
	status = pop3_response (pop3, NULL, 0, &len);
	POP3_CHECK_EAGAIN (pop3, status);
	if (strncasecmp (pop3->ack.buf, "+OK", 3) != 0)
	  {
	    stream_close (pop3->stream);
	    pop3->state = POP3_NO_STATE;
	    return MU_ERROR_OPERATION_DENIED;
	  }

	/* Get the timestamp.  */
	right = memchr (pop3->ack.buf, '<', len);
	if (right)
	  {
	    len = len - (right - pop3->ack.buf);
	    left = memchr (right, '>', len);
	    if (left)
	      {
		len = left - right;
		pop3->timestamp = calloc (len + 1, 1);
		if (pop3->timestamp == NULL)
		  {
		    stream_close (pop3->stream);
		    POP3_CHECK_ERROR (pop3, MU_ERROR_NO_MEMORY);
		  }
		/* Do not copy the surrounding '<>'.  */
		memcpy (pop3->timestamp, right + 1, len - 1);
	      }
	  }
	pop3->state = POP3_NO_STATE;
      }
    } /* End AUTHORISATION state. */

  return status;
}

/* GRRRRR!!  We can not use sleep in the library since this we'll
   muck up any alarm() done by the user.  */
static int
pop3_sleep (int seconds)
{
  struct timeval tval;
  tval.tv_sec = seconds;
  tval.tv_usec = 0;
  return select (1, NULL, NULL, NULL, &tval);
}

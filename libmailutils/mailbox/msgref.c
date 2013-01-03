/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
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

#include <config.h>
#include <stdlib.h>

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/monitor.h>
#include <mailutils/observer.h>
#include <mailutils/envelope.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/attribute.h>
#include <mailutils/stream.h>
#include <mailutils/sys/message.h>

void
mu_message_ref (mu_message_t msg)
{
  if (msg)
    {
      mu_monitor_wrlock (msg->monitor);
      msg->ref_count++;
      mu_monitor_unlock (msg->monitor);
    }
}

/* Free the message and all associated stuff */
static void
_mu_message_free (mu_message_t msg)
{
  /* Notify the listeners.  */
  /* FIXME: to be removed since we do not support this event.  */
  if (msg->observable)
    {
      mu_observable_notify (msg->observable, MU_EVT_MESSAGE_DESTROY, msg);
      mu_observable_destroy (&msg->observable, msg);
    }
      
  mu_envelope_destroy (&msg->envelope, msg);
  mu_header_destroy (&msg->header);
  mu_body_destroy (&msg->body, msg);
  mu_attribute_destroy (&msg->attribute, msg);
  mu_stream_destroy (&msg->rawstream);
  mu_stream_destroy (&msg->outstream);
  mu_mime_destroy (&msg->mime);
      
  /* Loose the owner.  */
  msg->owner = NULL;
      
  free (msg);
}

void
mu_message_unref (mu_message_t msg)
{
  if (msg)
    {
      mu_monitor_t monitor = msg->monitor;
      mu_monitor_wrlock (monitor);
      /* Note: msg->ref may be incremented by mu_message_ref without
	 additional checking for its owner, therefore decrementing
	 it must also occur independently of the owner checking. Due
	 to this inconsistency ref may reach negative values, which
	 is very unfortunate.
	 
	 The `owner' stuff is a leftover from older mailutils versions.
	 We are heading to removing it altogether. */
      if (msg->ref_count > 0)
	msg->ref_count--;
      if (msg->ref_count == 0)
	{
	  _mu_message_free (msg);
	  mu_monitor_unlock (monitor);
	  mu_monitor_destroy (&monitor, msg);
	}
      else
	mu_monitor_unlock (monitor);
    }
}

void
mu_message_destroy (mu_message_t *pmsg, void *owner)
{
  if (pmsg && *pmsg)
    {
      mu_message_t msg = *pmsg;
      
      mu_monitor_t monitor = msg->monitor;
      mu_monitor_wrlock (monitor);

      if (msg->owner && msg->owner == owner)
	{
	  _mu_message_free (msg);
	  mu_monitor_unlock (monitor);
	  mu_monitor_destroy (&monitor, msg);
	  *pmsg = NULL;
	  return;
	}
      mu_monitor_unlock (monitor);
    }
}


/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <mailutils/errno.h>
#include <envelope0.h>

int
mu_envelope_create (mu_envelope_t *penvelope, void *owner)
{
  mu_envelope_t envelope;
  if (penvelope == NULL)
    return MU_ERR_OUT_PTR_NULL;
  envelope = calloc (1, sizeof (*envelope));
  if (envelope == NULL)
    return ENOMEM;
  envelope->owner = owner;
  *penvelope = envelope;
  return 0;
}

void
mu_envelope_destroy (mu_envelope_t *penvelope, void *owner)
{
  if (penvelope && *penvelope)
    {
      mu_envelope_t envelope = *penvelope;
      if (envelope->owner == owner)
	{
	  if (envelope->_destroy)
	    envelope->_destroy (envelope);
	  free (envelope);
	}
      *penvelope = NULL;
    }
}

void *
mu_envelope_get_owner (mu_envelope_t envelope)
{
  return (envelope) ? envelope->owner : NULL;
}

int
mu_envelope_set_sender (mu_envelope_t envelope,
		   int (*_sender) (mu_envelope_t, char *, size_t, size_t*),
		   void *owner)
{
  if (envelope == NULL)
    return EINVAL;
  if (envelope->owner != owner)
    return EACCES;
  envelope->_sender = _sender;
  return 0;
}

int
mu_envelope_sender (mu_envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  if (envelope == NULL)
    return EINVAL;
  if (envelope->_sender)
    return envelope->_sender (envelope, buf, len, pnwrite);
  if (buf && len)
    *buf = '\0';
  if (pnwrite)
    *pnwrite = 0;
  return 0;
}

int
mu_envelope_set_date (mu_envelope_t envelope,
		   int (*_date) (mu_envelope_t, char *, size_t , size_t *),
		   void *owner)
{
  if (envelope == NULL)
    return EINVAL;
  if (envelope->owner != owner)
    return EACCES;
  envelope->_date = _date;
  return 0;
}

int
mu_envelope_date (mu_envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  if (envelope == NULL)
    return EINVAL;
  if (envelope->_date)
    return envelope->_date (envelope, buf, len, pnwrite);
  if (buf && len)
    *buf = '\0';
  if (pnwrite)
    *pnwrite = 0;
  return 0;
}

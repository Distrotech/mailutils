/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <errno.h>
#include <stdlib.h>
#include <envelope0.h>

int
envelope_create (envelope_t *penvelope, void *owner)
{
  envelope_t envelope;
  if (penvelope == NULL)
    return EINVAL;
  envelope = calloc (1, sizeof (*envelope));
  if (envelope == NULL)
    return ENOMEM;
  envelope->owner = owner;
  *penvelope = envelope;
  return 0;
}

void
envelope_destroy (envelope_t *penvelope, void *owner)
{
  if (penvelope && *penvelope)
    {
      envelope_t envelope = *penvelope;
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
envelope_get_owner (envelope_t envelope)
{
  return (envelope) ? envelope->owner : NULL;
}

int
envelope_set_sender (envelope_t envelope,
		   int (*_sender) __P ((envelope_t, char *, size_t, size_t*)),
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
envelope_sender (envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
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
envelope_set_date (envelope_t envelope,
		   int (*_date) __P ((envelope_t, char *, size_t , size_t *)),
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
envelope_date (envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
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

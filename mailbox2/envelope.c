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
#  include <config.h>
#endif

#include <mailutils/error.h>
#include <mailutils/sys/envelope.h>

int
(envelope_add_ref) (envelope_t envelope)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->add_ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->add_ref (envelope);
}

int
(envelope_release) (envelope_t envelope)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->release == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->release (envelope);
}

int
(envelope_destroy) (envelope_t envelope)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->destroy == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->destroy (envelope);
}

int
(envelope_sender) (envelope_t envelope, address_t *paddr)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->sender == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->sender (envelope, paddr);
}

int
(envelope_recipient) (envelope_t envelope, address_t *paddr)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->recipient == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->recipient (envelope, paddr);
}

int
(envelope_date) (envelope_t envelope, struct tm *tm, struct mu_timezone *tz)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->date == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->date (envelope, tm, tz);
}

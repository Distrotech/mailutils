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
envelope_ref (envelope_t envelope)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->ref (envelope);
}

void
envelope_destroy (envelope_t *penvelope)
{
  if (penvelope && *penvelope)
    {
      envelope_t envelope = *penvelope;
      if (envelope->vtable && envelope->vtable->destroy)
	envelope->vtable->destroy (penvelope);
      *penvelope = NULL;
    }
}

int
envelope_get_sender (envelope_t envelope, address_t *paddr)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->get_sender == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->get_sender (envelope, paddr);
}

int
envelope_set_sender (envelope_t envelope, address_t addr)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->set_sender == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->set_sender (envelope, addr);
}

int
envelope_get_recipient (envelope_t envelope, address_t *paddr)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->get_recipient == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->get_recipient (envelope, paddr);
}

int
envelope_set_recipient (envelope_t envelope, address_t addr)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->set_recipient == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->set_recipient (envelope, addr);
}

int
envelope_get_date (envelope_t envelope, struct tm *tm, struct mu_timezone *tz)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->get_date == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->get_date (envelope, tm, tz);
}

int
envelope_set_date (envelope_t envelope, struct tm *tm, struct mu_timezone *tz)
{
  if (envelope == NULL || envelope->vtable == NULL
      || envelope->vtable->set_date == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return envelope->vtable->set_date (envelope, tm, tz);
}

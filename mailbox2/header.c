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

#include <stdlib.h>
#include <string.h>
#include <mailutils/error.h>
#include <mailutils/sys/header.h>

int
header_ref (header_t header)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->ref (header);
}

void
header_destroy (header_t *pheader)
{
  if (pheader && *pheader)
    {
      header_t header = *pheader;
      if (header->vtable && header->vtable->destroy)
	header->vtable->destroy (pheader);
      *pheader = NULL;
    }
}

int
header_is_modified (header_t header)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->is_modified == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->is_modified (header);
}

int
header_clear_modified (header_t header)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->clear_modified == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->clear_modified (header);
}

int
header_set_value (header_t header, const char *fn, const char *fv, int replace)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->set_value == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->set_value (header, fn, fv, replace);
}

int
header_get_value (header_t header, const char *name, char *buffer,
		  size_t buflen, size_t *pn)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_value == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_value (header, name, buffer, buflen, pn);
}

int
header_aget_value (header_t header, const char *name, char **pvalue)
{
  char *value;
  size_t n = 0;
  int status = header_get_value (header, name, NULL, 0, &n);
  if (status == 0)
    {
      value = calloc (n + 1, 1);
      if (value == NULL)
        return MU_ERROR_NO_MEMORY;
      header_get_value (header, name, value, n + 1, NULL);
      *pvalue = value;
    }
  return status;
}

int
header_get_field_count (header_t header, size_t *pcount)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_field_count == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_field_count (header, pcount);
}

int
header_get_field_name (header_t header, size_t num, char *buf,
		       size_t buflen, size_t *pn)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_field_name == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_field_name (header, num, buf, buflen, pn);
}

int
header_aget_field_name (header_t header, size_t num, char **pvalue)
{
  char *value;
  size_t n = 0;
  int status = header_get_field_name (header, num, NULL, 0, &n);
  if (status == 0)
    {
      value = calloc (n + 1, 1);
      if (value == NULL)
        return MU_ERROR_NO_MEMORY;
      header_get_field_name (header, num, value, n + 1, NULL);
      *pvalue = value;
    }
  return status;
}

int
header_get_field_value (header_t header, size_t num, char *buf,
			size_t buflen, size_t *pn)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_field_value == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_field_value (header, num, buf, buflen, pn);
}

int
header_aget_field_value (header_t header, size_t num, char **pvalue)
{
  char *value;
  size_t n = 0;
  int status = header_get_field_value (header, num, NULL, 0, &n);
  if (status == 0)
    {
      value = calloc (n + 1, 1);
      if (value == NULL)
        return MU_ERROR_NO_MEMORY;
      header_get_field_value (header, num, value, n + 1, NULL);
      *pvalue = value;
    }
  return status;
}

int
header_get_lines (header_t header, size_t *plines)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_lines == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_lines (header, plines);
}

int
header_get_size (header_t header, size_t *psize)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_size == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_size (header, psize);
}

int
header_get_stream (header_t header, stream_t *pstream)
{
  if (header == NULL || header->vtable == NULL
      || header->vtable->get_stream == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return header->vtable->get_stream (header, pstream);
}

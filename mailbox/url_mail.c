/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <url0.h>
#include <registrar.h>
#include <errno.h>

static int url_mailto_create (url_t *purl, const char *name);
static void url_mailto_destroy (url_t *purl);

struct url_registrar _url_mailto_registrar =
{
  "mailto:",
  url_mailto_create, url_mailto_destroy
};

static void
url_mailto_destroy (url_t *purl)
{
  (void)purl;
  return;
}

static int
url_mailto_create (url_t *purl, const char *name)
{
  (void)purl; (void)name;
  return ENOSYS;
}

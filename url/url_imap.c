/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <url_imap.h>

struct url_type _url_imap_type =
{
  "imap://", 7,
  "imap://... TODO",
  (int)&_url_imap_type,
  url_imap_init, url_imap_destroy
};

void
url_imap_destroy (url_t *purl)
{
  return;
}

int
url_imap_init (url_t *purl, const char *name)
{
  return -1;
}

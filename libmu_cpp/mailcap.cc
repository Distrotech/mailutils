/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA
*/

#include <mailutils/cpp/mailcap.h>
#include <mailutils/cpp/stream.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Mailcap
//

Mailcap :: Mailcap (const Stream& stm)
{
  int status = mu_mailcap_create (&mailcap, stm.stm);
  if (status)
    throw Exception ("Mailcap::Mailcap", status);
}

Mailcap :: Mailcap (const mu_mailcap_t mailcap)
{
  if (mailcap == 0)
    throw Exception ("Mailcap::Mailcap", EINVAL);

  this->mailcap = mailcap;
}

Mailcap :: ~Mailcap ()
{
  mu_mailcap_destroy (&mailcap);
}

size_t
Mailcap :: getCount ()
{
  size_t count = 0;
  int status = mu_mailcap_entries_count (mailcap, &count);
  if (status)
    throw Exception ("Mailcap::getCount", status);
  return count;
}

MailcapEntry&
Mailcap :: getEntry (size_t i)
{
  mu_mailcap_entry_t c_entry;

  int status = mu_mailcap_get_entry (mailcap, i, &c_entry);
  if (status)
    throw Exception ("Mailcap::getEntry", status);

  MailcapEntry* entry = new MailcapEntry (c_entry);
  return *entry;
}

//
// MailcapEntry
//

MailcapEntry :: MailcapEntry (mu_mailcap_entry_t entry)
{
  if (entry == 0)
    throw Exception ("MailcapEntry::MailcapEntry", EINVAL);

  this->entry = entry;
}

size_t
MailcapEntry :: fieldsCount ()
{
  size_t count = 0;
  int status = mu_mailcap_entry_fields_count (entry, &count);
  if (status)
    throw Exception ("MailcapEntry::fieldsCount", status);
  return count;
}

std::string
MailcapEntry :: getField (size_t i)
{
  int status = mu_mailcap_entry_get_field (entry, i, buf, 
					   sizeof (buf), NULL);
  if (status)
    throw Exception ("MailcapEntry::getField", status);
  return std::string (buf);
}

std::string
MailcapEntry :: getTypeField ()
{
  int status = mu_mailcap_entry_get_typefield (entry, buf,
					       sizeof (buf), NULL);
  if (status)
    throw Exception ("MailcapEntry::getTypeField", status);
  return std::string (buf);
}

std::string
MailcapEntry :: getViewCommand ()
{
  int status = mu_mailcap_entry_get_viewcommand (entry, buf, 
						 sizeof (buf), NULL);
  if (status)
    throw Exception ("MailcapEntry::getViewCommand", status);
  return std::string (buf);
}


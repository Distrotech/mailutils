/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
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
Mailcap :: GetCount ()
{
  size_t count = 0;
  int status = mu_mailcap_entries_count (mailcap, &count);
  if (status)
    throw Exception ("Mailcap::GetCount", status);
  return count;
}

MailcapEntry&
Mailcap :: GetEntry (size_t i)
{
  mu_mailcap_entry_t c_entry;

  int status = mu_mailcap_get_entry (mailcap, i, &c_entry);
  if (status)
    throw Exception ("Mailcap::GetEntry", status);

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
MailcapEntry :: FieldsCount ()
{
  size_t count = 0;
  int status = mu_mailcap_entry_fields_count (entry, &count);
  if (status)
    throw Exception ("MailcapEntry::FieldsCount", status);
  return count;
}

std::string
MailcapEntry :: GetField (size_t i)
{
  int status = mu_mailcap_entry_get_field (entry, i, buf, 
					   sizeof (buf), NULL);
  if (status)
    throw Exception ("MailcapEntry::GetField", status);
  return std::string (buf);
}

std::string
MailcapEntry :: GetTypeField ()
{
  int status = mu_mailcap_entry_get_typefield (entry, buf,
					       sizeof (buf), NULL);
  if (status)
    throw Exception ("MailcapEntry::GetTypeField", status);
  return std::string (buf);
}

std::string
MailcapEntry :: GetViewCommand ()
{
  int status = mu_mailcap_entry_get_viewcommand (entry, buf, 
						 sizeof (buf), NULL);
  if (status)
    throw Exception ("MailcapEntry::GetViewCommand", status);
  return std::string (buf);
}


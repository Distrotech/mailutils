/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include <errno.h>
#include <string.h>

#include <mailutils/errno.h>
#include <mailutils/nls.h>

#ifndef EOK
# define EOK 0
#endif

const char*
mu_errname (int e)
{
  switch(e)
  {
#define EN(x)  case x: return #x

    EN(EOK);
    EN(EPERM);
    EN(ENOENT);
    EN(ESRCH);
    EN(EINTR);
    EN(EAGAIN);
    EN(ENOMEM);
    EN(EEXIST);
    EN(EINVAL);

    EN(MU_ERR_NO_HANDLER);
    EN(MU_ERR_EMPTY_VFN);

    EN(MU_ERR_OUT_NULL);
    EN(MU_ERR_OUT_PTR_NULL);

    EN(MU_ERR_MBX_NULL);

    EN(MU_ERR_BAD_822_FORMAT);
    EN(MU_ERR_EMPTY_ADDRESS);

    EN(MU_ERR_LOCKER_NULL);
    EN(MU_ERR_LOCK_CONFLICT);
    EN(MU_ERR_LOCK_BAD_LOCK);
    EN(MU_ERR_LOCK_BAD_FILE);
    EN(MU_ERR_LOCK_NOT_HELD);
    EN(MU_ERR_LOCK_EXT_FAIL);
    EN(MU_ERR_LOCK_EXT_ERR);
    EN(MU_ERR_LOCK_EXT_KILLED);

    EN(MU_ERR_NO_SUCH_USER);
    EN(MU_ERR_GETHOSTBYNAME);
    EN(MU_ERR_BAD_RESUMPTION);
    EN(MU_ERR_MAILER_BAD_FROM);
    EN(MU_ERR_MAILER_BAD_TO);
    EN(MU_ERR_MAILER_BAD_URL);
    EN(MU_ERR_MAILER_NO_RCPT_TO);
    EN(MU_ERR_SMTP_RCPT_FAILED);
    EN(MU_ERR_TCP_NO_HOST);
    EN(MU_ERR_TCP_NO_PORT);
    EN(MU_ERR_BAD_2047_INPUT);
  }

  return "SYSTEM ERROR";
}

const char *
mu_strerror (int e)
{
  switch(e)
  {
#define ES(x, d)  case x: return d
    ES(EOK,                     _("Success"));

    ES(MU_ERR_NO_HANDLER,       _("No registered handler"));
    ES(MU_ERR_EMPTY_VFN,        _("Empty virtual function"));

    ES(MU_ERR_OUT_NULL,         _("Pointer to output null"));
    ES(MU_ERR_OUT_PTR_NULL,     _("Pointer to output pointer null"));

    ES(MU_ERR_MBX_NULL,         _("Mailbox null"));

    ES(MU_ERR_BAD_822_FORMAT,   _("Format of RFC822 object is bad"));
    ES(MU_ERR_EMPTY_ADDRESS,    _("Address contains no addr specs"));

    ES(MU_ERR_LOCKER_NULL,      _("Locker null"));
    ES(MU_ERR_LOCK_CONFLICT,    _("Conflict with previous locker"));
    ES(MU_ERR_LOCK_BAD_LOCK,    _("Lock file check failed"));
    ES(MU_ERR_LOCK_BAD_FILE,    _("File check failed"));
    ES(MU_ERR_LOCK_NOT_HELD,    _("Lock not held on file"));
    ES(MU_ERR_LOCK_EXT_FAIL,    _("Failed to exec external locker"));
    ES(MU_ERR_LOCK_EXT_ERR,     _("External locker failed"));
    ES(MU_ERR_LOCK_EXT_KILLED,  _("External locker killed"));

    ES(MU_ERR_NO_SUCH_USER,     _("No such user name"));

    ES(MU_ERR_GETHOSTBYNAME,    _("DNS name resolution failed"));
    ES(MU_ERR_BAD_RESUMPTION,   _("State busy, must resume operation"));
    ES(MU_ERR_MAILER_BAD_FROM,  _("Not a valid mailer from address"));
    ES(MU_ERR_MAILER_BAD_TO,    _("Not a valid mailer to address"));
    ES(MU_ERR_MAILER_NO_RCPT_TO,_("No receipt addresses found"));
    ES(MU_ERR_MAILER_BAD_URL,   _("Malformed or unsupported mailer URL"));
    ES(MU_ERR_SMTP_RCPT_FAILED, _("SMTP rcpt to command failed"));
    ES(MU_ERR_TCP_NO_HOST,      _("Tcp connections need a host"));
    ES(MU_ERR_TCP_NO_PORT,      _("Tcp connections need a postive port"));

    ES(MU_ERR_BAD_2047_INPUT,   _("Input string is not RFC 2047 encoded"));
  }

  return strerror (e);
}



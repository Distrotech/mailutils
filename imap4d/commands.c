/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"

const struct imap4d_command imap4d_command_table [] = {
  { "capability",	imap4d_capability,	STATE_ALL },
  { "noop",		imap4d_noop,		STATE_ALL },
  { "logout",		imap4d_logout,		STATE_ALL },
  { "authenticate",	imap4d_authenticate,	STATE_NONAUTH },
  { "login",		imap4d_login,		STATE_NONAUTH },
  { "select",		imap4d_select,		STATE_AUTH | STATE_SEL },
  { "examine",		imap4d_examine,		STATE_AUTH | STATE_SEL },
  { "create",		imap4d_create,		STATE_AUTH | STATE_SEL },
  { "delete",		imap4d_delete,		STATE_AUTH | STATE_SEL },
  { "rename",		imap4d_rename,		STATE_AUTH | STATE_SEL },
  { "subscribe",	imap4d_subscribe,	STATE_AUTH | STATE_SEL },
  { "unsubscribe",	imap4d_unsubscribe,	STATE_AUTH | STATE_SEL },
  { "list",		imap4d_list,		STATE_AUTH | STATE_SEL },
  { "lsub",		imap4d_lsub,		STATE_AUTH | STATE_SEL },
  { "status",		imap4d_status,		STATE_AUTH | STATE_SEL },
  { "append",		imap4d_append,		STATE_AUTH | STATE_SEL },
  { "check",		imap4d_check,		STATE_SEL },
  { "close",		imap4d_close,		STATE_SEL },
  { "expunge",		imap4d_expunge,		STATE_SEL },
  { "search",		imap4d_search,		STATE_SEL },
  { "fetch",		imap4d_fetch,		STATE_SEL },
  { "store",		imap4d_store,		STATE_SEL },
  { "copy",		imap4d_copy,		STATE_SEL },
  { "uid",		imap4d_uid,		STATE_SEL },
  { 0, 0, 0}
};

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
  { "capability", imap4d_capability, STATE_ALL, STATE_NONE, STATE_NONE },
  { "noop", imap4d_noop, STATE_ALL, STATE_NONE, STATE_NONE },
  { "logout", imap4d_logout, STATE_ALL, STATE_LOGOUT, STATE_NONE },
  { "authenticate", imap4d_authenticate, STATE_NONAUTH, STATE_NONE, STATE_AUTH },
  { "login", imap4d_login, STATE_NONAUTH, STATE_NONE, STATE_AUTH },
  { "select", imap4d_select, STATE_AUTH | STATE_SEL, STATE_AUTH, STATE_SEL },
  { "examine", imap4d_examine, STATE_AUTH | STATE_SEL, STATE_AUTH, STATE_SEL },
  { "create", imap4d_create, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "delete", imap4d_delete, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "rename", imap4d_rename, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "subscribe", imap4d_subscribe, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "unsubscribe", imap4d_unsubscribe, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "list", imap4d_list, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "lsub", imap4d_lsub, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "status", imap4d_status, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "append", imap4d_append, STATE_AUTH | STATE_SEL, STATE_NONE, STATE_NONE },
  { "check", imap4d_check, STATE_SEL, STATE_NONE, STATE_NONE },
  { "close", imap4d_close, STATE_SEL, STATE_AUTH, STATE_AUTH },
  { "expunge", imap4d_expunge, STATE_SEL, STATE_NONE, STATE_NONE },
  { "search", imap4d_search, STATE_SEL, STATE_NONE, STATE_NONE },
  { "fetch", imap4d_fetch, STATE_SEL, STATE_NONE, STATE_NONE },
  { "store", imap4d_store, STATE_SEL, STATE_NONE, STATE_NONE },
  { "copy", imap4d_copy, STATE_SEL, STATE_NONE, STATE_NONE },
  { "uid", imap4d_uid, STATE_SEL, STATE_NONE, STATE_NONE },
  { 0, 0, 0}
};

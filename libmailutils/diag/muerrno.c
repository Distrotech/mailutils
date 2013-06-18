/* -*- c -*- This file is generated automatically. Please, do not edit.
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
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
  static char buf[128];
  
  switch (e)
    {
    case EOK:
      return "EOK";

      case MU_ERR_FAILURE:
        return "MU_ERR_FAILURE";

      case MU_ERR_CANCELED:
        return "MU_ERR_CANCELED";

      case MU_ERR_EMPTY_VFN:
        return "MU_ERR_EMPTY_VFN";

      case MU_ERR_OUT_PTR_NULL:
        return "MU_ERR_OUT_PTR_NULL";

      case MU_ERR_MBX_REMOVED:
        return "MU_ERR_MBX_REMOVED";

      case MU_ERR_NOT_OPEN:
        return "MU_ERR_NOT_OPEN";

      case MU_ERR_OPEN:
        return "MU_ERR_OPEN";

      case MU_ERR_INVALID_EMAIL:
        return "MU_ERR_INVALID_EMAIL";

      case MU_ERR_EMPTY_ADDRESS:
        return "MU_ERR_EMPTY_ADDRESS";

      case MU_ERR_LOCKER_NULL:
        return "MU_ERR_LOCKER_NULL";

      case MU_ERR_LOCK_CONFLICT:
        return "MU_ERR_LOCK_CONFLICT";

      case MU_ERR_LOCK_BAD_LOCK:
        return "MU_ERR_LOCK_BAD_LOCK";

      case MU_ERR_LOCK_BAD_FILE:
        return "MU_ERR_LOCK_BAD_FILE";

      case MU_ERR_LOCK_NOT_HELD:
        return "MU_ERR_LOCK_NOT_HELD";

      case MU_ERR_LOCK_EXT_FAIL:
        return "MU_ERR_LOCK_EXT_FAIL";

      case MU_ERR_LOCK_EXT_ERR:
        return "MU_ERR_LOCK_EXT_ERR";

      case MU_ERR_LOCK_EXT_KILLED:
        return "MU_ERR_LOCK_EXT_KILLED";

      case MU_ERR_NO_SUCH_USER:
        return "MU_ERR_NO_SUCH_USER";

      case MU_ERR_GETHOSTBYNAME:
        return "MU_ERR_GETHOSTBYNAME";

      case MU_ERR_MAILER_BAD_FROM:
        return "MU_ERR_MAILER_BAD_FROM";

      case MU_ERR_MAILER_BAD_TO:
        return "MU_ERR_MAILER_BAD_TO";

      case MU_ERR_MAILER_NO_RCPT_TO:
        return "MU_ERR_MAILER_NO_RCPT_TO";

      case MU_ERR_MAILER_BAD_URL:
        return "MU_ERR_MAILER_BAD_URL";

      case MU_ERR_SMTP_RCPT_FAILED:
        return "MU_ERR_SMTP_RCPT_FAILED";

      case MU_ERR_TCP_NO_HOST:
        return "MU_ERR_TCP_NO_HOST";

      case MU_ERR_TCP_NO_PORT:
        return "MU_ERR_TCP_NO_PORT";

      case MU_ERR_BAD_2047_INPUT:
        return "MU_ERR_BAD_2047_INPUT";

      case MU_ERR_BAD_2047_ENCODING:
        return "MU_ERR_BAD_2047_ENCODING";

      case MU_ERR_NOUSERNAME:
        return "MU_ERR_NOUSERNAME";

      case MU_ERR_NOPASSWORD:
        return "MU_ERR_NOPASSWORD";

      case MU_ERR_BADREPLY:
        return "MU_ERR_BADREPLY";

      case MU_ERR_SEQ:
        return "MU_ERR_SEQ";

      case MU_ERR_REPLY:
        return "MU_ERR_REPLY";

      case MU_ERR_BAD_AUTH_SCHEME:
        return "MU_ERR_BAD_AUTH_SCHEME";

      case MU_ERR_AUTH_FAILURE:
        return "MU_ERR_AUTH_FAILURE";

      case MU_ERR_PROCESS_NOEXEC:
        return "MU_ERR_PROCESS_NOEXEC";

      case MU_ERR_PROCESS_EXITED:
        return "MU_ERR_PROCESS_EXITED";

      case MU_ERR_PROCESS_SIGNALED:
        return "MU_ERR_PROCESS_SIGNALED";

      case MU_ERR_PROCESS_UNKNOWN_FAILURE:
        return "MU_ERR_PROCESS_UNKNOWN_FAILURE";

      case MU_ERR_CONN_CLOSED:
        return "MU_ERR_CONN_CLOSED";

      case MU_ERR_PARSE:
        return "MU_ERR_PARSE";

      case MU_ERR_NOENT:
        return "MU_ERR_NOENT";

      case MU_ERR_EXISTS:
        return "MU_ERR_EXISTS";

      case MU_ERR_BUFSPACE:
        return "MU_ERR_BUFSPACE";

      case MU_ERR_SQL:
        return "MU_ERR_SQL";

      case MU_ERR_DB_ALREADY_CONNECTED:
        return "MU_ERR_DB_ALREADY_CONNECTED";

      case MU_ERR_DB_NOT_CONNECTED:
        return "MU_ERR_DB_NOT_CONNECTED";

      case MU_ERR_RESULT_NOT_RELEASED:
        return "MU_ERR_RESULT_NOT_RELEASED";

      case MU_ERR_NO_QUERY:
        return "MU_ERR_NO_QUERY";

      case MU_ERR_BAD_COLUMN:
        return "MU_ERR_BAD_COLUMN";

      case MU_ERR_NO_RESULT:
        return "MU_ERR_NO_RESULT";

      case MU_ERR_NO_INTERFACE:
        return "MU_ERR_NO_INTERFACE";

      case MU_ERR_BADOP:
        return "MU_ERR_BADOP";

      case MU_ERR_BAD_FILENAME:
        return "MU_ERR_BAD_FILENAME";

      case MU_ERR_READ:
        return "MU_ERR_READ";

      case MU_ERR_NO_TRANSPORT:
        return "MU_ERR_NO_TRANSPORT";

      case MU_ERR_AUTH_NO_CRED:
        return "MU_ERR_AUTH_NO_CRED";

      case MU_ERR_URL_MISS_PARTS:
        return "MU_ERR_URL_MISS_PARTS";

      case MU_ERR_URL_EXTRA_PARTS:
        return "MU_ERR_URL_EXTRA_PARTS";

      case MU_ERR_URL_INVALID_PARAMETER:
        return "MU_ERR_URL_INVALID_PARAMETER";

      case MU_ERR_INFO_UNAVAILABLE:
        return "MU_ERR_INFO_UNAVAILABLE";

      case MU_ERR_NONAME:
        return "MU_ERR_NONAME";

      case MU_ERR_BADFLAGS:
        return "MU_ERR_BADFLAGS";

      case MU_ERR_SOCKTYPE:
        return "MU_ERR_SOCKTYPE";

      case MU_ERR_FAMILY:
        return "MU_ERR_FAMILY";

      case MU_ERR_SERVICE:
        return "MU_ERR_SERVICE";

      case MU_ERR_PERM_OWNER_MISMATCH:
        return "MU_ERR_PERM_OWNER_MISMATCH";

      case MU_ERR_PERM_GROUP_WRITABLE:
        return "MU_ERR_PERM_GROUP_WRITABLE";

      case MU_ERR_PERM_WORLD_WRITABLE:
        return "MU_ERR_PERM_WORLD_WRITABLE";

      case MU_ERR_PERM_GROUP_READABLE:
        return "MU_ERR_PERM_GROUP_READABLE";

      case MU_ERR_PERM_WORLD_READABLE:
        return "MU_ERR_PERM_WORLD_READABLE";

      case MU_ERR_PERM_LINKED_WRDIR:
        return "MU_ERR_PERM_LINKED_WRDIR";

      case MU_ERR_PERM_DIR_IWGRP:
        return "MU_ERR_PERM_DIR_IWGRP";

      case MU_ERR_PERM_DIR_IWOTH:
        return "MU_ERR_PERM_DIR_IWOTH";

      case MU_ERR_DISABLED:
        return "MU_ERR_DISABLED";

      case MU_ERR_FORMAT:
        return "MU_ERR_FORMAT";

    }

  snprintf (buf, sizeof buf, _("Error %d"), e);
  return buf;
}

const char *
mu_strerror (int e)
{
  switch (e)
    {
    case EOK:
      return _("Success");

    case MU_ERR_FAILURE:
      return _("Operation failed");

    case MU_ERR_CANCELED:
      return _("Operation canceled");

    case MU_ERR_EMPTY_VFN:
      return _("Empty virtual function");

    case MU_ERR_OUT_PTR_NULL:
      return _("Null output pointer");

    case MU_ERR_MBX_REMOVED:
      return _("Mailbox removed");

    case MU_ERR_NOT_OPEN:
      return _("Resource not open");

    case MU_ERR_OPEN:
      return _("Resource is already open");

    case MU_ERR_INVALID_EMAIL:
      return _("Malformed email address");

    case MU_ERR_EMPTY_ADDRESS:
      return _("Empty address list");

    case MU_ERR_LOCKER_NULL:
      return _("Locker null");

    case MU_ERR_LOCK_CONFLICT:
      return _("Conflict with previous locker");

    case MU_ERR_LOCK_BAD_LOCK:
      return _("Lock file check failed");

    case MU_ERR_LOCK_BAD_FILE:
      return _("File check failed");

    case MU_ERR_LOCK_NOT_HELD:
      return _("Lock not held on file");

    case MU_ERR_LOCK_EXT_FAIL:
      return _("Failed to execute external locker");

    case MU_ERR_LOCK_EXT_ERR:
      return _("External locker failed");

    case MU_ERR_LOCK_EXT_KILLED:
      return _("External locker killed");

    case MU_ERR_NO_SUCH_USER:
      return _("No such user name");

    case MU_ERR_GETHOSTBYNAME:
      return _("DNS name resolution failed");

    case MU_ERR_MAILER_BAD_FROM:
      return _("Not a valid sender address");

    case MU_ERR_MAILER_BAD_TO:
      return _("Not a valid recipient address");

    case MU_ERR_MAILER_NO_RCPT_TO:
      return _("No recipient addresses found");

    case MU_ERR_MAILER_BAD_URL:
      return _("Malformed or unsupported mailer URL");

    case MU_ERR_SMTP_RCPT_FAILED:
      return _("SMTP RCPT command failed");

    case MU_ERR_TCP_NO_HOST:
      return _("Required host specification is missing");

    case MU_ERR_TCP_NO_PORT:
      return _("Invalid port or service specification");

    case MU_ERR_BAD_2047_INPUT:
      return _("Input string is not RFC 2047 encoded");

    case MU_ERR_BAD_2047_ENCODING:
      return _("Not a valid RFC 2047 encoding");

    case MU_ERR_NOUSERNAME:
      return _("User name is not supplied");

    case MU_ERR_NOPASSWORD:
      return _("User password is not supplied");

    case MU_ERR_BADREPLY:
      return _("Invalid reply from the remote host");

    case MU_ERR_SEQ:
      return _("Bad command sequence");

    case MU_ERR_REPLY:
      return _("Operation rejected by remote party");

    case MU_ERR_BAD_AUTH_SCHEME:
      return _("Unsupported authentication scheme");

    case MU_ERR_AUTH_FAILURE:
      return _("Authentication failed");

    case MU_ERR_PROCESS_NOEXEC:
      return _("Cannot execute");

    case MU_ERR_PROCESS_EXITED:
      return _("Process exited with a non-zero status");

    case MU_ERR_PROCESS_SIGNALED:
      return _("Process exited on signal");

    case MU_ERR_PROCESS_UNKNOWN_FAILURE:
      return _("Unknown failure while executing subprocess");

    case MU_ERR_CONN_CLOSED:
      return _("Connection closed by remote host");

    case MU_ERR_PARSE:
      return _("Parse error");

    case MU_ERR_NOENT:
      return _("Requested item not found");

    case MU_ERR_EXISTS:
      return _("Item already exists");

    case MU_ERR_BUFSPACE:
      return _("Not enough buffer space");

    case MU_ERR_SQL:
      return _("SQL error");

    case MU_ERR_DB_ALREADY_CONNECTED:
      return _("Already connected to the database");

    case MU_ERR_DB_NOT_CONNECTED:
      return _("Not connected to the database");

    case MU_ERR_RESULT_NOT_RELEASED:
      return _("Result of the previous query is not released");

    case MU_ERR_NO_QUERY:
      return _("No query was yet executed");

    case MU_ERR_BAD_COLUMN:
      return _("Bad column address");

    case MU_ERR_NO_RESULT:
      return _("No result from the previous query available");

    case MU_ERR_NO_INTERFACE:
      return _("No such interface");

    case MU_ERR_BADOP:
      return _("Inappropriate operation for this mode");

    case MU_ERR_BAD_FILENAME:
      return _("Badly formed file or directory name");

    case MU_ERR_READ:
      return _("Read error");

    case MU_ERR_NO_TRANSPORT:
      return _("Transport stream not set");

    case MU_ERR_AUTH_NO_CRED:
      return _("No credentials supplied");

    case MU_ERR_URL_MISS_PARTS:
      return _("URL missing required parts");

    case MU_ERR_URL_EXTRA_PARTS:
      return _("URL has parts not allowed by its scheme");

    case MU_ERR_URL_INVALID_PARAMETER:
      return _("Invalid parameter in URL");

    case MU_ERR_INFO_UNAVAILABLE:
      return _("Information is not yet available");

    case MU_ERR_NONAME:
      return _("Name or service not known");

    case MU_ERR_BADFLAGS:
      return _("Bad value for flags");

    case MU_ERR_SOCKTYPE:
      return _("Socket type not supported");

    case MU_ERR_FAMILY:
      return _("Address family not supported");

    case MU_ERR_SERVICE:
      return _("Requested service not supported");

    case MU_ERR_PERM_OWNER_MISMATCH:
      return _("File owner mismatch");

    case MU_ERR_PERM_GROUP_WRITABLE:
      return _("Group writable file");

    case MU_ERR_PERM_WORLD_WRITABLE:
      return _("World writable file");

    case MU_ERR_PERM_GROUP_READABLE:
      return _("Group readable file");

    case MU_ERR_PERM_WORLD_READABLE:
      return _("World readable file");

    case MU_ERR_PERM_LINKED_WRDIR:
      return _("Linked file in a writable directory");

    case MU_ERR_PERM_DIR_IWGRP:
      return _("File in group writable directory");

    case MU_ERR_PERM_DIR_IWOTH:
      return _("File in world writable directory");

    case MU_ERR_DISABLED:
      return _("Requested feature disabled in configuration");

    case MU_ERR_FORMAT:
      return _("Error in format string");

    }

  return strerror (e);
}


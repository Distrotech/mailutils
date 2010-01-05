/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007, 2010 Free Software Foundation,
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
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAPI_H
#define _MAPI_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef char * LPTSTR;
typedef void * LPVOID;

typedef unsigned long   ULONG;
typedef unsigned long * LPULONG;
typedef unsigned long   FLAGS;
typedef unsigned long   LHANDLE;
typedef unsigned long * LPLHANDLE;

#define MAPI_OLE          1
#define MAPI_OLE_STATIC   2

/* Constant values.  */
#define MAPI_ORIG   0
#define MAPI_TO     1
#define MAPI_CC     2
#define MAPI_BCC    3

/* FIXME: What are the values ? */
#define MAPI_RECEIPT_REQUESTED
#define MAPI_SENT
#define MAPI_UNREAD

#define MAPI_FORCE_DOWNLOAD
#define MAPI_NEW_SESSION
#define MAPI_LOGON_UI
#define MAPI_PASSWORD_UI

#define MAPI_GUARANTEE_FIFO
#define MAPI_LONG_MSGID
#define MAPI_UNREAD_ONLY

#define MAPI_BODY_AS_FILE
#define MAPI_ENVELOPE_ONLY
#define MAPI_PEEK
#define MAPI_SUPPRESS_ATTACH

#define MAPI_AB_NOMODIFY
#define MAPI_LOGON_UI
#define MAPI_NEW_SESSION
#define MAPI_DIALOG

typedef struct
{
  ULONG  ulReserved;
  ULONG  flFlags;
  ULONG  nPosition;
  LPTSTR lpszPathName;
  LPTSTR lpszFileName;
  LPVOID lpFileType;
} MapiFileDesc, *lpMapiFileDesc;

typedef struct
{
  ULONG  ulReserved;
  ULONG  ulRecipClass;
  LPTSTR lpszName;
  LPTSTR lpszAddress;
  ULONG  ulEIDSize;
  LPVOID lpEntryID;
} MapiRecipDesc, *lpMapiRecipDesc;


typedef struct
{
  ULONG  ulReserved;
  LPTSTR lpszSubject;
  LPTSTR lpszNoteText;
  LPTSTR lpszMessageType;
  LPTSTR lpszDateReceived;
  LPTSTR lpszConversationID;
  FLAGS  flFlags;
  lpMapiRecipDesc lpOriginator;
  ULONG  nRecipCount;
  lpMapiRecipDesc lpRecips;
  ULONG  nFileCount;
  lpMapiFileDesc lpFiles;
} MapiMessage, *lpMapiMessage;

ULONG MAPILogon (ULONG ulUIParam, LPTSTR lpszProfileName,
		 LPTSTR lpszPassword, FLAGS flFlags, ULONG ulReserved,
		 LPLHANDLE lplhSession);


ULONG MAPILogoff (LHANDLE lhSession, ULONG ulUIParam, FLAGS flFlags,
		  ULONG ulReserved);

ULONG MAPIFreeBuffer (LPVOID lpBuffer);

ULONG MAPISendMail (LHANDLE lhSession, ULONG ulUIParam,
		    lpMapiMessage lpMessage, FLAGS flFlags,
		    ULONG ulReserved);

ULONG MAPISendDocuments (ULONG ulUIParam, LPTSTR lpszDelimChar,
			 LPTSTR lpszFullPaths, LPTSTR lpszFileNames,
			 ULONG ulReserved);

ULONG MAPIFindNext (LHANDLE lhSession, ULONG ulUIParam,
		    LPTSTR lpszMessageType, LPTSTR lpszSeedMessageID,
		    FLAGS flFlags, ULONG ulReserved, LPTSTR lpszMessageID);

ULONG MAPIReadMail (LHANDLE lhSession, ULONG ulUIParam,
		    LPTSTR lpszMessageID, FLAGS flFlags, ULONG ulReserved,
		    lpMapiMessage * lppMessage);


ULONG MAPISaveMail (LHANDLE lhSession, ULONG ulUIParam,
		    lpMapiMessage lpMessage, FLAGS flFlags,
		    ULONG ulReserved, LPTSTR lpszMessageID);

ULONG MAPIDeleteMail (LHANDLE lhSession, ULONG ulUIParam,
		      LPTSTR lpszMessageID, FLAGS flFlags,
		      ULONG ulReserved);

ULONG MAPIAddress (LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszCaption,
		   ULONG nEditFields, LPTSTR lpszLabels, ULONG nRecips,
		   lpMapiRecipDesc lpRecips, FLAGS flFlags,
		   ULONG ulReserved, LPULONG lpnNewRecips,
		   lpMapiRecipDesc * lppNewRecips);

ULONG MAPIDetails (LHANDLE lhSession, ULONG ulUIParam,
		   lpMapiRecipDesc lpRecip, FLAGS flFlags,
		   ULONG ulReserved);

ULONG MAPIResolveName (LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszName,
		       FLAGS flFlags, ULONG ulReserved,
		       lpMapiRecipDesc * lppRecip);

#define SUCCESS_SUCCESS                         0
#define MAPI_USER_ABORT                         1
#define MAPI_E_FAILURE                          2
#define MAPI_E_LOGIN_FAILURE                    3
#define MAPI_E_DISK_FULL                        4
#define MAPI_E_INSUFFICIENT_MEMORY              5
#define MAPI_E_ACCESS_DENIED                    6
#define MAPI_E_TOO_MANY_SESSIONS                8
#define MAPI_E_TOO_MANY_FILES                   9
#define MAPI_E_TOO_MANY_RECIPIENTS             10
#define MAPI_E_ATTACHMENT_NOT_FOUND            11
#define MAPI_E_ATTACHMENT_OPEN_FAILURE         12
#define MAPI_E_ATTACHMENT_WRITE_FAILURE        13
#define MAPI_E_UNKNOWN_RECIPIENT               14
#define MAPI_E_BAD_RECIPTYPE                   15
#define MAPI_E_NO_MESSAGES                     16
#define MAPI_E_INVALID_MESSAGE                 17
#define MAPI_E_TEXT_TOO_LARGE                  18
#define MAPI_E_INVALID_SESSION                 19
#define MAPI_E_TYPE_NOT_SUPPORTED              20
#define MAPI_E_AMBIGUOUS_RECIPIENT             21
#define MAPI_E_MESSAGE_IN_USE                  22
#define MAPI_E_NETWORK_FAILURE                 23
#define MAPI_E_INVALID_EDITFIELDS              24
#define MAPI_E_INVALID_RECIPS                  25
#define MAPI_E_NOT_SUPPORTED                   26

#ifdef __cplusplus
}
#endif

#endif	/* _MAPI_H */

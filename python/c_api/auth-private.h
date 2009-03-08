/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA
*/

#ifndef _MUCAPI_AUTH_H
#define _MUCAPI_AUTH_H

#include <mailutils/auth.h>
#include <mailutils/mu_auth.h>

typedef struct {
  PyObject_HEAD;
  mu_authority_t auth;
} PyAuthority;

typedef struct {
  PyObject_HEAD;
  mu_ticket_t ticket;
} PyTicket;

typedef struct {
  PyObject_HEAD;
  mu_wicket_t wicket;
} PyWicket;

typedef struct {
  PyObject_HEAD;
  struct mu_auth_data *auth_data;
} PyAuthData;

extern PyAuthority * PyAuthority_NEW ();
extern int PyAuthority_Check (PyObject *x);

extern PyTicket * PyTicket_NEW ();
extern int PyTicket_Check (PyObject *x);

extern PyWicket * PyWicket_NEW ();
extern int PyWicket_Check (PyObject *x);

extern PyAuthData * PyAuthData_NEW ();
extern int PyAuthData_Check (PyObject *x);

#endif /* not _MUCAPI_AUTH_H */

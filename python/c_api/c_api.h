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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Python.h>

#define PY_PACKAGE_NAME "mailutils"
#define PY_ROOT_NAME "c_api"
#define PY_PACKAGE_VERSION PACKAGE_VERSION

extern inline PyObject * _ro (PyObject *obj);
extern void py_dealloc (PyObject *self);
extern PyObject * status_object (int status, PyObject *py_obj);
extern PyObject * attach_module (const char *name, PyMethodDef *methods);

extern void init_error ();
extern void init_address ();
extern void init_attribute ();
extern void init_auth ();
extern void init_body ();
extern void init_debug ();
extern void init_envelope ();
extern void init_header  ();
extern void init_filter ();
extern void init_folder ();
extern void init_mailer ();
extern void init_mailbox ();
extern void init_mailcap ();
extern void init_message ();
extern void init_mime ();
extern void init_registrar ();
extern void init_stream ();
extern void init_url ();
extern void init_util ();

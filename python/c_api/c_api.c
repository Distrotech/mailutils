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

#include "c_api.h"

inline PyObject *
_ro (PyObject *obj)
{
  Py_INCREF (obj);
  return obj;
}

void
py_dealloc (PyObject *self)
{
  self->ob_type->tp_free (self);
}

PyObject *
status_object (int status, PyObject *py_obj)
{
  PyObject *py_ret = PyTuple_New (2);
  PyTuple_SetItem (py_ret, 0, PyInt_FromLong (status));
  PyTuple_SetItem (py_ret, 1, py_obj);
  return _ro (py_ret);
}

static PyMethodDef nomethods[] = {
  { NULL, NULL }
};

static PyObject *package;
static PyObject *all;

PyObject *
attach_module (const char *name, PyMethodDef *methods)
{
  PyObject *module, *m;

  char ns[64] = PY_PACKAGE_NAME "." PY_ROOT_NAME ".";
  strcat (ns, name);

  if (!(module = PyImport_AddModule (ns)))
    return NULL;

  if (PyModule_AddObject (package, name, module))
    return NULL;

  Py_INCREF (module);

  if (!(m = Py_InitModule (ns, methods)))
    return NULL;

  PyList_Append (all, PyString_FromString (name));
  return m;
}

PyMODINIT_FUNC
initc_api (void)
{
  package = Py_InitModule (PY_ROOT_NAME, nomethods);
  if (!package)
    return;

  PyModule_AddStringConstant (package, "__version__", PY_PACKAGE_VERSION);
  if (!PyModule_AddObject (package, "__all__", _ro (PyList_New (0))))
    {
      all = PyObject_GetAttrString (package, "__all__");
      if (!all || !PyList_Check (all))
	return;
    }

  init_error ();
  init_address ();
  init_attribute ();
  init_auth ();
  init_body ();
  init_debug ();
  init_envelope ();
  init_header ();
  init_filter ();
  init_folder ();
  init_mailer ();
  init_mailbox ();
  init_mailcap ();
  init_message ();
  init_mime ();
  init_registrar ();
  init_stream ();
  init_url ();
  init_util ();
}

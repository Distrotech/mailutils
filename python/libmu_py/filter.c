/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#include "libmu_py.h"

#define PY_MODULE "filter"

static PyObject *
api_filter_create (PyObject *self, PyObject *args)
{
  int status, mode, flags;
  char *code = NULL;
  PyStream *py_stm, *py_transport;

  if (!PyArg_ParseTuple (args, "OOsii",
			 &py_stm, &py_transport,
			 &code, &mode, &flags))
    return NULL;

  if (!PyStream_Check ((PyObject *)py_stm) &&
      !PyStream_Check ((PyObject *)py_transport))
    {
      PyErr_SetString (PyExc_TypeError, "");
      return NULL;
    }

  status = mu_filter_create (&py_stm->stm, py_transport->stm,
			     code, mode, flags);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_filter_iconv_create (PyObject *self, PyObject *args)
{
  int status, flags;
  char *fromcode = NULL;
  char *tocode = NULL;
  PyStream *py_stm, *py_transport;
  const char *argv[4] = { "iconv", NULL, NULL, NULL };
  
  if (!PyArg_ParseTuple (args, "OOssi",
			 &py_stm, &py_transport,
			 &fromcode, &tocode, &flags))
    return NULL;

  if (!PyStream_Check ((PyObject *)py_stm) &&
      !PyStream_Check ((PyObject *)py_transport))
    {
      PyErr_SetString (PyExc_TypeError, "");
      return NULL;
    }

  argv[1] = fromcode;
  argv[2] = tocode;
  status = mu_filter_create_args (&py_stm->stm, py_transport->stm, argv[0], 3,
				  argv, MU_FILTER_DECODE, flags);
  return _ro (PyInt_FromLong (status));
}

static PyMethodDef methods[] = {
  { "create", (PyCFunction) api_filter_create, METH_VARARGS,
    "" },

  { "iconv_create", (PyCFunction) api_filter_iconv_create, METH_VARARGS,
    "" },

  { NULL, NULL, 0, NULL }
};

void
_mu_py_attach_filter (void)
{
  _mu_py_attach_module (PY_MODULE, methods);
}

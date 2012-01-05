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

#define PY_MODULE "stream"
#define PY_CSNAME "StreamType"

static PyObject *
_repr (PyObject *self)
{
  char buf[80];
  sprintf (buf, "<" PY_MODULE "." PY_CSNAME " instance at %p>", self);
  return PyString_FromString (buf);
}

static PyTypeObject PyStreamType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /* ob_size */
  PY_MODULE "." PY_CSNAME,   /* tp_name */
  sizeof (PyStream),         /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)_py_dealloc,   /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr; __getattr__ */
  0,                         /* tp_setattr; __setattr__ */
  0,                         /* tp_compare; __cmp__ */
  _repr,                     /* tp_repr; __repr__ */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash; __hash__ */
  0,                         /* tp_call; __call__ */
  _repr,                     /* tp_str; __str__ */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,        /* tp_flags */
  "",                        /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  0,                         /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  0,                         /* tp_init */
  0,                         /* tp_alloc */
  0,                         /* tp_new */
};

PyStream *
PyStream_NEW ()
{
  return (PyStream *)PyObject_NEW (PyStream, &PyStreamType);
}

int
PyStream_Check (PyObject *x)
{
  return x->ob_type == &PyStreamType;
}

static PyObject *
api_tcp_stream_create (PyObject *self, PyObject *args)
{
  int status, flags, port;
  char *host;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!sii", &PyStreamType, &py_stm,
			 &host, &port, &flags))
    return NULL;

  status = mu_tcp_stream_create (&py_stm->stm, host, port, flags);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_file_stream_create (PyObject *self, PyObject *args)
{
  int status, flags;
  char *filename;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!si", &PyStreamType, &py_stm,
			 &filename, &flags))
    return NULL;

  status = mu_file_stream_create (&py_stm->stm, filename, flags);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_stdio_stream_create (PyObject *self, PyObject *args)
{
  int status, fd, flags;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!ii", &PyStreamType, &py_stm, &fd, &flags))
    return NULL;

  status = mu_stdio_stream_create (&py_stm->stm, fd, flags);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_memory_stream_create (PyObject *self, PyObject *args)
{
  int status;
  char *s;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!s", &PyStreamType, &py_stm, &s))
    return NULL;

  status = mu_memory_stream_create (&py_stm->stm, MU_STREAM_RDWR);
  if (status == 0)
    {
      status = mu_stream_write (py_stm->stm, s, strlen (s), NULL);
      mu_stream_seek (py_stm->stm, MU_SEEK_SET, 0, NULL);
    }
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_prog_stream_create (PyObject *self, PyObject *args)
{
  int status, flags;
  char *progname;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!si", &PyStreamType, &py_stm,
			 &progname, &flags))
    return NULL;

  status = mu_command_stream_create (&py_stm->stm, progname, flags);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_filter_prog_stream_create (PyObject *self, PyObject *args)
{
  int status;
  char *progname;
  PyStream *py_stm, *py_input;
  struct mu_prog_hints hints;
  struct mu_wordsplit ws;

  if (!PyArg_ParseTuple (args, "O!sO!",
			 &PyStreamType, &py_stm,
			 &progname,
			 &PyStreamType, &py_input))
    return NULL;

  if (mu_wordsplit (progname, &ws, MU_WRDSF_DEFFLAGS))
    return _ro (PyInt_FromLong (errno));

  hints.mu_prog_input = py_input->stm;
  status = mu_prog_stream_create (&py_stm->stm, ws.ws_wordv[0],
				  ws.ws_wordc, ws.ws_wordv,
				  MU_PROG_HINT_INPUT,
				  &hints,
				  MU_STREAM_READ);
  mu_wordsplit_free (&ws);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_stream_ref (PyObject *self, PyObject *args)
{
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  mu_stream_ref (py_stm->stm);
  return _ro (Py_None);
}

static PyObject *
api_stream_unref (PyObject *self, PyObject *args)
{
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  mu_stream_unref (py_stm->stm);
  return _ro (Py_None);
}

static PyObject *
api_stream_destroy (PyObject *self, PyObject *args)
{
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  mu_stream_destroy (&py_stm->stm);
  return _ro (Py_None);
}

static PyObject *
api_stream_open (PyObject *self, PyObject *args)
{
  int status;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  status = mu_stream_open (py_stm->stm);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_stream_close (PyObject *self, PyObject *args)
{
  int status;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  status = mu_stream_close (py_stm->stm);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_stream_flush (PyObject *self, PyObject *args)
{
  int status;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  status = mu_stream_flush (py_stm->stm);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_stream_wait (PyObject *self, PyObject *args)
{
  int status, wflags;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!i", &PyStreamType, &py_stm, &wflags))
    return NULL;

  status = mu_stream_wait (py_stm->stm, &wflags, NULL);
  return _ro (PyInt_FromLong (status));
}

static PyObject *
api_stream_read (PyObject *self, PyObject *args)
{
  int status;
  size_t read_count;
  char rbuf[1024];
  PyObject *py_ret;
  PyStream *py_stm;

  memset (rbuf, 0, sizeof (rbuf));

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  status = mu_stream_read (py_stm->stm, rbuf, sizeof (rbuf), &read_count);

  py_ret = PyTuple_New (3);
  PyTuple_SetItem (py_ret, 0, PyInt_FromLong (status));
  PyTuple_SetItem (py_ret, 1, PyString_FromString (rbuf));
  PyTuple_SetItem (py_ret, 2, PyInt_FromLong (read_count));
  return _ro (py_ret);
}

static PyObject *
api_stream_write (PyObject *self, PyObject *args)
{
  int status;
  char *wbuf;
  size_t size, write_count;
  PyStream *py_stm;

  if (!PyArg_ParseTuple (args, "O!si", &PyStreamType, &py_stm, &wbuf, &size))
    return NULL;

  status = mu_stream_write (py_stm->stm, wbuf, size, &write_count);
  return status_object (status, PyInt_FromLong (write_count));
}

static PyObject *
api_stream_readline (PyObject *self, PyObject *args)
{
  int status;
  size_t read_count;
  char rbuf[1024];
  PyObject *py_ret;
  PyStream *py_stm;

  memset (rbuf, 0, sizeof (rbuf));

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  status = mu_stream_readline (py_stm->stm, rbuf, sizeof (rbuf), &read_count);

  py_ret = PyTuple_New (3);
  PyTuple_SetItem (py_ret, 0, PyInt_FromLong (status));
  PyTuple_SetItem (py_ret, 1, PyString_FromString (rbuf));
  PyTuple_SetItem (py_ret, 2, PyInt_FromLong (read_count));
  return _ro (py_ret);
}

static PyObject *
api_stream_to_message (PyObject *self, PyObject *args)
{
  int status;
  PyStream *py_stm;
  PyMessage *py_msg = PyMessage_NEW ();

  if (!PyArg_ParseTuple (args, "O!", &PyStreamType, &py_stm))
    return NULL;

  Py_INCREF (py_msg);

  status = mu_stream_to_message (py_stm->stm, &py_msg->msg);
  return status_object (status, (PyObject *)py_msg);
}

static PyMethodDef methods[] = {
  { "tcp_stream_create", (PyCFunction) api_tcp_stream_create, METH_VARARGS,
    "" },

  { "file_stream_create", (PyCFunction) api_file_stream_create, METH_VARARGS,
    "" },

  { "stdio_stream_create", (PyCFunction) api_stdio_stream_create, METH_VARARGS,
    "" },

  { "memory_stream_create",
    (PyCFunction) api_memory_stream_create, METH_VARARGS,
    "" },

  { "prog_stream_create", (PyCFunction) api_prog_stream_create, METH_VARARGS,
    "" },

  { "filter_prog_stream_create",
    (PyCFunction) api_filter_prog_stream_create, METH_VARARGS,
    "" },

  { "ref", (PyCFunction) api_stream_ref, METH_VARARGS,
    "" },

  { "unref", (PyCFunction) api_stream_unref, METH_VARARGS,
    "" },

  { "destroy", (PyCFunction) api_stream_destroy, METH_VARARGS,
    "" },

  { "open", (PyCFunction) api_stream_open, METH_VARARGS,
    "" },

  { "close", (PyCFunction) api_stream_close, METH_VARARGS,
    "" },

  { "flush", (PyCFunction) api_stream_flush, METH_VARARGS,
    "" },

  { "wait", (PyCFunction) api_stream_wait, METH_VARARGS,
    "" },

  { "read", (PyCFunction) api_stream_read, METH_VARARGS,
    "" },

  { "write", (PyCFunction) api_stream_write, METH_VARARGS,
    "" },

  { "readline", (PyCFunction) api_stream_readline, METH_VARARGS,
    "" },

  { "to_message", (PyCFunction) api_stream_to_message, METH_VARARGS,
    "" },

  { NULL, NULL, 0, NULL }
};

int
mu_py_init_stream (void)
{
  PyStreamType.tp_new = PyType_GenericNew;
  return PyType_Ready (&PyStreamType);
}

void
_mu_py_attach_stream (void)
{
  PyObject *m;
  if ((m = _mu_py_attach_module (PY_MODULE, methods)))
    {
      Py_INCREF (&PyStreamType);
      PyModule_AddObject (m, PY_CSNAME, (PyObject *)&PyStreamType);
    }
}

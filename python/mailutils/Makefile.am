## This file is part of GNU Mailutils.
## Copyright (C) 2009-2012 Free Software Foundation, Inc.
##
## GNU Mailutils is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 3, or (at
## your option) any later version.
##
## GNU Mailutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. 

pythonsitedir=@PYTHON_SITE_DIR@
pythonsite_PYTHON=\
 __init__.py \
 error.py \
 address.py \
 attribute.py \
 auth.py \
 body.py \
 envelope.py \
 filter.py \
 folder.py \
 header.py \
 mailer.py \
 mailbox.py \
 mailcap.py \
 message.py \
 mime.py \
 nls.py \
 registrar.py \
 secret.py \
 sieve.py \
 stream.py \
 url.py \
 util.py

pythonexecdir=@PYTHON_EXEC_DIR@
pythonexec_LTLIBRARIES = c_api.la
c_api_la_LDFLAGS = -avoid-version -module -rpath $(pythonexecdir)
c_api_la_LIBADD = $(PYTHON_LIBS) ${MU_LIB_PY}
c_api_la_SOURCES = c_api.c

INCLUDES = @MU_LIB_COMMON_INCLUDES@ @PYTHON_INCLUDES@\
 -I$(top_srcdir)/python/libmu_py

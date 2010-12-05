# GNU Mailutils -- a suite of utilities for electronic mail
# Copyright (C) 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009,
# 2010 Free Software Foundation, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this library.  If not, see
# <http://www.gnu.org/licenses/>.

SUBDIRS = auth base address cfg diag filter mailbox mailer mime\
 server string stream stdstream property url . tests 

lib_LTLIBRARIES = libmailutils.la

libmailutils_la_SOURCES =  

libmailutils_la_LIBADD = \
 @MU_COMMON_LIBRARIES@\
 auth/libauth.la\
 base/libbase.la\
 address/libaddress.la\
 cfg/libcfg.la\
 diag/libdiag.la\
 filter/libfilter.la\
 mailbox/libmailbox.la\
 mailer/libmailer.la\
 mime/libmime.la\
 property/libproperty.la\
 server/libserver.la\
 string/libstring.la\
 stream/libstream.la\
 stdstream/libstdstream.la\
 url/liburl.la

libmailutils_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@


dnl This file is part of GNU Mailutils.  -*- autoconf -*-
dnl Copyright (C) 2011-2012 Free Software Foundation, Inc.
dnl
dnl GNU Mailutils is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl GNU Mailutils is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([MU_ENABLE_IPV6],
   [AC_ARG_ENABLE(ipv6,                     
     [AC_HELP_STRING([--enable-ipv6], [enable IPv6 support])],
     [status_ipv6=$enableval],
     [status_ipv6=maybe])
     
    if test $status_ipv6 != no; then
      working_ipv6=no
      AC_EGREP_CPP(MAILUTILS_AF_INET6_DEFINED,[
#include <sys/socket.h>
#if defined(AF_INET6)
MAILUTILS_AF_INET6_DEFINED
#endif
],[working_ipv6=yes])

      AC_CHECK_TYPE([struct sockaddr_storage],
                    [working_ipv6=yes], [working_ipv6=no],
		    [#include <sys/socket.h>])
      AC_CHECK_TYPE([struct sockaddr_in6],
                    [working_ipv6=yes], [working_ipv6=no],
		    [#include <sys/types.h>
                     #include <netinet/in.h>])
      AC_CHECK_TYPE([struct addrinfo],
                    [working_ipv6=yes], [working_ipv6=no],
		    [#include <netdb.h>])
      AC_CHECK_FUNC([getnameinfo],
                    [working_ipv6=yes], [working_ipv6=no],
		    [#include <netdb.h>])

      if test $working_ipv6 = no; then
	if test $status_ipv6 = yes; then
	  AC_MSG_ERROR([IPv6 support is required but not available])
	fi
      fi
      status_ipv6=$working_ipv6
      if test $status_ipv6 = yes; then
	AC_DEFINE_UNQUOTED([MAILUTILS_IPV6],1,
	                   [Define to 1 if IPv6 support is enabled])
      fi
    fi])
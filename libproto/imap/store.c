/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_store (mu_imap_t imap, int uid, const char *msgset, const char *items)
{
  char const *argv[3];
  static struct imap_command com;

  argv[0] = "STORE";
  argv[1] = msgset;
  argv[2] = items;
  
  com.session_state = MU_IMAP_SESSION_SELECTED;
  com.capa = NULL;
  com.rx_state = MU_IMAP_CLIENT_STORE_RX;
  com.uid = uid;
  com.argc = 3;
  com.argv = argv;
  com.handler = NULL;

  return mu_imap_gencom (imap, &com);
}
      

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/* ta[g] [msglist] */
/* unt[ag] [msglist] */

static int
tag_message (message_t mesg, msgset_t *msgset, void *arg)
{
  attribute_t attr;
  int *action = arg;

  (void)msgset;
  message_get_attribute (mesg, &attr);
  if (*action)
    attribute_set_userflag (attr, MAIL_ATTRIBUTE_TAGGED);
  else
    attribute_unset_userflag (attr, MAIL_ATTRIBUTE_TAGGED);
  return 0;
}

int
mail_tag (int argc, char **argv)
{
  msgset_t *msgset;
  int action = argv[0][0] != 'u';

  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &msgset))
    return 1;

  util_msgset_iterate (msgset, tag_message, (void *)&action);

  msgset_free (msgset);
  return 0;
}

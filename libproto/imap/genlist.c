/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
#include <mailutils/address.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/list.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

struct list_closure
{
  int error_code;
  mu_list_t retlist;
  const char *command;
};

static int
count_level (const char *name, int delim)
{
  int level = 0;

  while (*name)
    if (*name++ == delim)
      level++;
  return level;
}

static int
list_attr_conv (void *item, void *data)
{
  struct imap_list_element *elt = item;
  struct mu_list_response *rp = data;
  
  if (elt->type != imap_eltype_string)
    return 0;
  if (mu_c_strcasecmp (elt->v.string, "\\Noinferiors"))
    rp->type |= MU_FOLDER_ATTRIBUTE_DIRECTORY;
  if (mu_c_strcasecmp (elt->v.string, "\\Noselect"))
    rp->type |= MU_FOLDER_ATTRIBUTE_FILE;
  /* FIXME: \Marked nad \Unmarked have no correspondence in flags. */
  return 0;
}
  
static void
list_untagged_handler (mu_imap_t imap, mu_list_t resp, void *data)
{
  struct list_closure *clos = data;
  struct imap_list_element *elt;
  size_t count;
  
  if (clos->error_code)
    return;

  mu_list_count (resp, &count);
  if (count == 4 &&
      _mu_imap_list_nth_element_is_string (resp, 0, clos->command))
    {
      struct mu_list_response *rp;

      rp = calloc (1, sizeof (*rp));
      if (!rp)
	{
	  clos->error_code = ENOMEM;
	  return;
	}
	  
      elt = _mu_imap_list_at (resp, 1);
      if (!(elt && elt->type == imap_eltype_list))
	return;
      rp->type = 0;
      mu_list_foreach (elt->v.list, list_attr_conv, rp);

      elt = _mu_imap_list_at (resp, 3);
      if (!(elt && elt->type == imap_eltype_string))
	return;
      rp->name = strdup (elt->v.string);
      if (!rp->name)
	{
	  free (rp);
	  clos->error_code = ENOMEM;
	  return;
	}

      elt = _mu_imap_list_at (resp, 2);
      if (!elt)
	return;
      if (_mu_imap_list_element_is_nil (elt))
	{
	  rp->separator = 0;
	  rp->level = 0;
	}
      else if (elt->type != imap_eltype_string)
	return;
      else
	{
	  rp->separator = elt->v.string[0];
	  rp->level = count_level (rp->name, rp->separator);
	}
      if ((clos->error_code = mu_list_append (clos->retlist, rp)))
	mu_list_response_free (rp);
    }
}
  
int
mu_imap_genlist (mu_imap_t imap, int lsub,
		 const char *refname, const char *mboxname,
		 mu_list_t retlist)
{
  char const *argv[3];
  static struct imap_command com;
  struct list_closure clos;
  int rc;

  if (!refname || !mboxname)
    return EINVAL;
  
  argv[0] = lsub ? "LSUB" : "LIST";
  argv[1] = refname;
  argv[2] = mboxname;

  clos.error_code = 0;
  clos.retlist = retlist;
  clos.command = argv[0];
  
  com.session_state = MU_IMAP_SESSION_AUTH;
  com.capa = NULL;
  com.rx_state = lsub ? MU_IMAP_CLIENT_LSUB_RX : MU_IMAP_CLIENT_LIST_RX;
  com.argc = 3;
  com.argv = argv;
  com.extra = NULL;
  com.msgset = NULL;
  com.tagged_handler = NULL;
  com.untagged_handler = list_untagged_handler;
  com.untagged_handler_data = &clos;

  rc = mu_imap_gencom (imap, &com);
  if (rc == 0)
    rc = clos.error_code;

  return rc;
}

int
mu_imap_genlist_new (mu_imap_t imap, int lsub,
		     const char *refname, const char *mboxname,
		     mu_list_t *plist)
{
  mu_list_t list;
  int rc = mu_list_create (&list);
  if (rc == 0)
    {
      mu_list_set_destroy_item (list, mu_list_response_free);
      rc = mu_imap_genlist (imap, lsub, refname, mboxname, list);
      if (rc)
	mu_list_destroy (&list);
      else
	*plist = list;
    }
  return rc;
}

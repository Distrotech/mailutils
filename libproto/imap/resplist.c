/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>

static void
_imap_list_free (void *ptr)
{
  struct imap_list_element *elt = ptr;

  switch (elt->type)
    {
    case imap_eltype_string:
      free (elt->v.string);
      break;
      
    case imap_eltype_list:
      mu_list_destroy (&elt->v.list);
    }
  free (ptr);
}

static int
_mu_imap_response_list_create (mu_imap_t imap, mu_list_t *plist)
{
  mu_list_t list;
  int status = mu_list_create (&list);
  MU_IMAP_CHECK_ERROR (imap, status);
  mu_list_set_destroy_item (list, _imap_list_free);
  *plist = list;
  return 0;
}

#define IS_LBRACE(p) ((p)[0] == '(' && !(p)[1])
#define IS_RBRACE(p) ((p)[0] == ')' && !(p)[1])
#define IS_NIL(p) (strcmp (p, "NIL") == 0)

static struct imap_list_element *
_new_imap_list_element (mu_imap_t imap, enum imap_eltype type)
{
  struct imap_list_element *elt = calloc (1, sizeof (*elt));
  if (!elt)
    {
      imap->client_state = MU_IMAP_CLIENT_ERROR;
    }
  else
    elt->type = type;
  return elt;
}

struct parsebuf
{
  mu_imap_t pb_imap;
  size_t pb_count;
  char **pb_arr;
  int pb_err;
  int pb_inlist;
};

static void
parsebuf_init (struct parsebuf *pb, mu_imap_t imap)
{
  memset (pb, 0, sizeof *pb);
  pb->pb_imap = imap;
}

static int
parsebuf_advance (struct parsebuf *pb)
{
  if (pb->pb_count == 0)
    return MU_ERR_NOENT;
  pb->pb_count--;
  pb->pb_arr++;
  return 0;
}

static char *
parsebuf_gettok (struct parsebuf *pb)
{
  char *p;
  
  if (pb->pb_count == 0)
    return NULL;
  p = *pb->pb_arr;
  parsebuf_advance (pb);
  return p;
}

static char *
parsebuf_peek (struct parsebuf *pb)
{
  if (pb->pb_count == 0)
    return NULL;
  return *pb->pb_arr;
}

static void
parsebuf_seterr (struct parsebuf *pb, int err)
{
  pb->pb_err = err;
}

static struct imap_list_element *_parse_element (struct parsebuf *pb);

static struct imap_list_element *
_parse_list (struct parsebuf *pb)
{
  int rc;
  struct imap_list_element *elt, *list_elt;

  elt = _new_imap_list_element (pb->pb_imap, imap_eltype_list);
  if (!elt)
    {
      parsebuf_seterr (pb, ENOMEM);
      return NULL;
    }

  rc = _mu_imap_response_list_create (pb->pb_imap, &elt->v.list);
  if (rc)
    {
      free (elt);
      parsebuf_seterr (pb, rc);
      return NULL;
    }

  while ((list_elt = _parse_element (pb)))
    mu_list_append (elt->v.list, list_elt);

  return elt;
}

static struct imap_list_element *
_parse_element (struct parsebuf *pb)
{
  struct imap_list_element *elt;
  char *tok;

  if (pb->pb_err)
    return NULL;

  tok = parsebuf_gettok (pb);
  
  if (!tok)
    {
      if (pb->pb_inlist)
	parsebuf_seterr (pb, MU_ERR_PARSE);
      return NULL;
    }
  
  if (IS_LBRACE (tok))
    {
      tok = parsebuf_peek (pb);
      if (!tok)
	{
	  parsebuf_seterr (pb, MU_ERR_PARSE);
	  return NULL;
	}
      
      if (IS_RBRACE (tok))
	{
	  parsebuf_gettok (pb);
	  elt = _new_imap_list_element (pb->pb_imap, imap_eltype_list);
	  if (!elt)
	    {
	      parsebuf_seterr (pb, ENOMEM);
	      return NULL;
	    }
	  elt->v.list = NULL;
	}
      else
	{
	  pb->pb_inlist++;
	  elt = _parse_list (pb);
	}
    }
  else if (IS_RBRACE (tok))
    {
      if (pb->pb_inlist)
	pb->pb_inlist--;
      else
	parsebuf_seterr (pb, MU_ERR_PARSE);
      return NULL;
    }
  else if (IS_NIL (tok))
    {
      elt = _new_imap_list_element (pb->pb_imap, imap_eltype_list);
      if (!elt)
	{
	  parsebuf_seterr (pb, ENOMEM);
	  return NULL;
	}
      elt->v.list = NULL;
    }
  else
    {
      char *s;
      elt = _new_imap_list_element (pb->pb_imap, imap_eltype_string);
      if (!elt)
	{
	  parsebuf_seterr (pb, ENOMEM);
	  return NULL;
	}
      s = strdup (tok);
      if (!s)
	{
	  free (elt);
	  parsebuf_seterr (pb, ENOMEM);
	  return NULL;
	}
      elt->v.string = s;
    }
  return elt;
}

int
_mu_imap_untagged_response_to_list (mu_imap_t imap, mu_list_t *plist)
{
  struct imap_list_element *elt;
  struct parsebuf pb;

  parsebuf_init (&pb, imap);
  mu_imapio_get_words (imap->io, &pb.pb_count, &pb.pb_arr);
  parsebuf_advance (&pb); /* Skip initial '*' */
  elt = _parse_list (&pb);
  if (pb.pb_err)
    {
      if (elt)
	_imap_list_free (elt);
      imap->client_state = MU_IMAP_CLIENT_ERROR;
      return pb.pb_err;
    }
  *plist = elt->v.list;
  free (elt);
  return 0;
}

int
_mu_imap_list_element_is_string (struct imap_list_element *elt,
				 const char *str)
{
  if (elt->type != imap_eltype_string)
    return 0;
  return strcmp (elt->v.string, str) == 0;
}

int
_mu_imap_list_element_is_nil (struct imap_list_element *elt)
{
  return elt->type == imap_eltype_list && mu_list_is_empty (elt->v.list);
}

struct imap_list_element *
_mu_imap_list_at (mu_list_t list, int idx)
{
  struct imap_list_element *arg;
  int rc = mu_list_get (list, idx, (void*) &arg);
  if (rc)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		("%s:%d: cannot get list element: %s",
		 __FILE__, __LINE__, mu_strerror (rc)));
      return NULL;
    }
  return arg;
}

int
_mu_imap_list_nth_element_is_string (mu_list_t list, size_t n,
				     const char *str)
{
  struct imap_list_element *elt = _mu_imap_list_at (list, n);
  return elt && elt->type == imap_eltype_string &&
	 strcmp (elt->v.string, str) == 0;
}

int
_mu_imap_list_nth_element_is_string_ci (mu_list_t list, size_t n,
				     	const char *str)
{
  struct imap_list_element *elt = _mu_imap_list_at (list, n);
  return elt && elt->type == imap_eltype_string &&
	 mu_c_strcasecmp (elt->v.string, str) == 0;
}



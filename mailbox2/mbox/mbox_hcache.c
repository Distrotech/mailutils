/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))


static const char *hcache_default[] =
{
  "Bcc",
  "Cc",
  "Content-Language",
  "Content-Transfer-Encoding",
  "Content-Type",
  "Date",
  "From",
  "In-Reply-To",
  "Mail-Followup-To",
  "Message-ID",
  "Reply-To",
  "Return-Path",
  "Sender",
  "Subject",
  "To",
  "X-UIDL"
};

void
mbox_release_hcache (mbox_t mbox, unsigned int msgno)
{
  struct _hcache *hc;

  if (mbox == NULL)
    return;

  if (msgno)
    hc = &mbox->umessages[msgno - 1]->hcache;
  else
    hc = &mbox->hcache;

  if (hc->size)
    {
      unsigned int i;
      for (i = 0; i < hc->size; i++)
	{
	  if (hc->values[i])
	    free (hc->values[i]);
	}
    }
  hc->size = 0;

  if (hc->values)
    free (hc->values);
  hc->values = NULL;
}

int
mbox_set_hcache (mbox_t mbox, const char **array, size_t len)
{
  int status = 0;

  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  mbox_release_hcache (mbox, 0);

  if (array && len)
    status = mbox_add_hcache (mbox, array, len);

  return status;
}

int
mbox_add_hcache (mbox_t mbox, const char **array, size_t len)
{
  size_t i;
  int status = 0;

  if (mbox == NULL || array == NULL || len == 0)
    return MU_ERROR_INVALID_PARAMETER;

  for (i = 0; i < len; i++)
    {
      char **values;
      if (array[i] == NULL || array[i][0] == '\0')
	continue;
      values = realloc (mbox->hcache.values,
			(mbox->hcache.size + 1) * sizeof (*values));
      if (values == NULL)
	{
	  status = MU_ERROR_NO_MEMORY;
	  break;
	}
      mbox->hcache.values = values;
      mbox->hcache.values[mbox->hcache.size] = strdup (array[i]);
      mbox->hcache.values[mbox->hcache.size][0] =
	toupper (mbox->hcache.values[mbox->hcache.size][0]);
      mbox->hcache.size++;
    }
  return status;
}

int
mbox_append_hcache (mbox_t mbox, unsigned int msgno, const char *name,
		    const char *value)
{
  struct _hcache *hc;
  size_t i;
  int status = MU_ERROR_ENTRY_NOT_EXIST;

  if (mbox == NULL || msgno == 0 || name == NULL || *name == '\0')
    return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->messages_count)
    return MU_ERROR_INVALID_PARAMETER;

  if (mbox->hcache.size == 0)
    return MU_ERROR_INVALID_PARAMETER;

  msgno--;
  hc = &(mbox->umessages[msgno]->hcache);
  if (hc->values == NULL)
    {
      hc->values = calloc (mbox->hcache.size, sizeof (*(mbox->hcache.values)));
      if (hc->values == NULL)
	return MU_ERROR_NO_MEMORY;
    }
  hc->size = mbox->hcache.size;

  for (i = 0; i < mbox->hcache.size; i++)
    {
      if (mbox->hcache.values[i]
	  && toupper (*name) == mbox->hcache.values[i][0]
	  && strcasecmp (mbox->hcache.values[i], name) == 0)
	{
	  if (value == NULL)
	    {
	      if (hc->values[i])
		free (hc->values[i]);
	      hc->values[i] = NULL;
	    }
	  else
	    {
	      if (hc->values[i] == NULL)
		hc->values[i] = strdup (value);
	      else
		{
		  char *tmp = realloc (hc->values[i], strlen (value) +
				       strlen (hc->values[i]) + 1);
		  if (tmp)
		    {
		      hc->values[i] = tmp;
		      strcat (hc->values[i], value);
		    }
		}
	    }
	  status = 0;
	  break;
	}
    }
  return status;
}

int
mbox_set_default_hcache (mbox_t mbox)
{
  return mbox_set_hcache (mbox, hcache_default,
			  (sizeof hcache_default) / sizeof (*hcache_default));
}

int
mbox_value_hcache (mbox_t mbox, unsigned int msgno, const char *name,
		   char *buf, size_t buflen, size_t *pn)
{
  mbox_message_t mum;
  size_t i;
  size_t n = 0;
  int status = MU_ERROR_ENTRY_NOT_EXIST;

  mbox_debug_print (mbox, "value_hcache(%d)", msgno);

  if (mbox == NULL || msgno == 0 || name == NULL || *name == '\0')
    return MU_ERROR_INVALID_PARAMETER;

  if (msgno > mbox->umessages_count)
    return MU_ERROR_INVALID_PARAMETER;

  mum = mbox->umessages[msgno - 1];
  for (i = 0; i < mbox->hcache.size; i++)
    {
      if (mbox->hcache.values[i]
	  && toupper (*name) == mbox->hcache.values[i][0]
	  && strcasecmp (mbox->hcache.values[i], name) == 0)
	{
	  if (mum->hcache.size == mbox->hcache.size)
	    {
	      n = strlen (mum->hcache.values[i]);
	      if (buf && buflen)
		{
		  buflen--;
		  n = min (buflen, n);
		  memcpy (buf, mum->hcache.values[i], n);
		  buf[n] = '\0';
		}
	      status = 0;
	    }
	}
    }

  if (pn)
    *pn = n;
  return status;
}

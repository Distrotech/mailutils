/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

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
  "Message-ID",
  "Reply-To",
  "Reply-To",
  "Sender",
  "Subject",
  "To",
  "X-UIDL"
};

void
mbox_hcache_free (mbox_t mbox, unsigned int msgno)
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
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  mbox_hcache_free (mbox, 0);

  if (array && len)
    {
      unsigned int i;

      mbox->hcache.values = calloc (len, sizeof (*(mbox->hcache.values)));

      if (mbox->hcache.values == NULL)
	return MU_ERROR_NO_MEMORY;

      for (i = 0; i < len; i++)
	{
	  mbox->hcache.values[i] = strdup (array[i]);
	  if (mbox->hcache.values[i] == NULL)
	    {
	      mbox_set_hcache (mbox, NULL, 0);
	      return MU_ERROR_NO_MEMORY;
	    }
	  mbox->hcache.size++;
	}
    }
  return 0;
}

int
mbox_hcache_append (mbox_t mbox, unsigned int msgno, const char *name,
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

  for (i = 0; i < mbox->hcache.size; i++)
    {
      if (strcasecmp (mbox->hcache.values[i], name) == 0)
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
		      strcat (hc->values[i], value);
		      hc->values[i] = tmp;
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
mbox_set_hcache_default (mbox_t mbox)
{
  return mbox_set_hcache (mbox, hcache_default,
			  sizeof (hcache_default) / sizeof (*hcache_default));
}

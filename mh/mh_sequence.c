/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <mh.h>

static char *
private_sequence_name (char *name)
{
  char *p;
  char *mbox_dir = mh_expand_name (NULL, current_folder, 0);
  asprintf (&p, "atr-%s-%s", name, mbox_dir);
  free (mbox_dir);
  return p;
}

char *
mh_seq_read (char *name, int flags)
{
  char *value;

  if (flags & SEQ_PRIVATE)
    {
      char *p = private_sequence_name (name);
      value = mh_global_context_get (p, NULL);
      free (p);
    }
  else
    value = mh_global_sequences_get (name, NULL);
  return value;
}

static void
write_sequence (char *name, char *value, int private)
{
  if (private)
    {
      char *p = private_sequence_name (name);
      mh_global_context_set (p, value);
      free (p);
    }
  else
    mh_global_sequences_set (name, value);
}

static void
delete_sequence (char *name, int private)
{
  write_sequence (name, NULL, private);
}

void
mh_seq_add (char *name, mh_msgset_t *mset, int flags)
{
  char *value = mh_seq_read (name, flags);
  char *new_value, *p;
  char buf[64];
  size_t i, len;

  delete_sequence (name, !(flags & SEQ_PRIVATE));

  if (flags & SEQ_ZERO)
    value = NULL;
  
  if (value)
    len = strlen (value);
  else
    len = 0;
  len++;
  for (i = 0; i < mset->count; i++)
    {
      snprintf (buf, sizeof buf, "%lu", (unsigned long) mset->list[i]);
      len += strlen (buf) + 1;
    }

  new_value = xmalloc (len + 1);
  if (value)
    strcpy (new_value, value);
  else
    new_value[0] = 0;
  p = new_value + strlen (new_value);
  *p++ = ' ';
  for (i = 0; i < mset->count; i++)
    {
      p += sprintf (p, "%lu", (unsigned long) mset->list[i]);
      *p++ = ' ';
    }
  *p = 0;
  write_sequence (name, new_value, flags & SEQ_PRIVATE);
  if (strcasecmp (name, "cur") == 0)
    current_message = strtoul (new_value, NULL, 0);
  free (new_value);
}

static int
cmp_msgnum (const void *a, const void *b)
{
  const size_t *as = a;
  const size_t *bs = b;

  if (*as < *bs)
    return -1;
  if (*as > *bs)
    return 1;
  return 0;
}

int
mh_seq_delete (char *name, mh_msgset_t *mset, int flags)
{
  char *value = mh_seq_read (name, flags);
  char *p;
  int argc, i, count;
  char **argv;
  
  if (!value)
    return 0;

  if (argcv_get (value, "", NULL, &argc, &argv))
    return 0;

  for (i = 0; i < argc; i++)
    {
      char *p;
      size_t num = strtoul (argv[i], &p, 10);

      if (*p)
	continue;

      if (bsearch (&num, mset->list, mset->count, sizeof (mset->list[0]),
		   cmp_msgnum))
	{
	  free (argv[i]);
	  argv[i] = NULL;
	}
    }

  p = value;
  count = 0;
  for (i = 0; i < argc; i++)
    {
      if (argv[i])
	{
	  strcpy (p, argv[i]);
	  p += strlen (p);
	  *p++ = ' ';
	  count++;
	}
    }
  *p = 0;
  write_sequence (name, count > 0 ? value : NULL, flags & SEQ_PRIVATE);
  argcv_free (argc, argv);
  
  return 0;
}


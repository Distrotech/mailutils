/* GNU mailutils - a suite of utilities for electronic mail
 * Copyright (C) 1999, 2000 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Library Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef int _url_mh_type;

struct mailbox_type _mailbox_mh_type =
{
  "MH"
  (int)&_url_mh_type, &_url_mh_type,
  mailbox_mh_init, mailbox_mh_destroy
};

typedef struct _mh_data
{
  time_t mtime; /* used for checking if mailbox was updated */
} mh_data;

static int mh_sequence(const char *name);

int
mailbox_mh_init (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  mh_data *data;

  mbox = malloc(sizeof(*mbox));
  data = malloc(sizeof(mh_data));
  mbox->name = malloc(strlen(name) + 1);
  strcpy(mbox->name, name);
  mbox->data = data;
  *pmbox = mbox;

  return 0;
}

void
mailbox_mh_destroy (mailbox_t *pmbox)
{
  free((*pmbox)->data);
  free((*pmbox)->name);
  free(*pmbox);
  pmbox = NULL;
}

/* FIXME: handle create once the design is ready */

/* mh_scan actually does all the dirty work */
static int
mh_open (mailbox_t mbox, int flags)
{
  struct stat st;
  mh_data *data;

  if (stat(mbox->name, &st) == -1)
    return errno;

  if (! S_ISDIR(st.st_mode))
    return EINVAL; /* mailbox is not a directory, thus it is also not MH */

  data = mbox->data;
  /* FIXME: does mtime change when the dir has a new file added? */
  data->mtime = st.st_mtime;

  return 0; /* everything is fine */
}

static int
mh_close (mailbox_t mbox)
{
  mh_data *data;

  data = mbox->data;

  /* FIXME: implementation */
  return 0;
}

static int
mh_scan (mailbox_t mbox, size_t *msgs)
{
  struct stat st;
  DIR *maildir;
  struct dirent *entry;
  mh_data *data;
  unsigned int count = 0;
  int parse_sequence_file = 0;

  data = mbox->data;

  maildir = opendir(mbox->name);

  while((entry = readdir(maildir)) != NULL) {
    if(strcmp(".", entry->d_name) == 0 ||
       strcmp("..", entry->d_name) == 0)
      continue;
    /* FIXME: handle this MH extension */
    if(entry->d_name[0] == '+')
	  continue;
	/* FIXME: decide what to do with messages marked for removal */
	if(entry->d_name[0] == ',')
      continue;
	if(entry->d_name[0] == '.') {
      if(strcmp(".mh_sequences", entry->d_name))
        continue; /* spurious file beginning w/ '.' */
      else { /* MH info in .mh_sequences */
        /* FIXME: parse this file */
        parse_sequence_file = 1;
      }
    }
	if(mh_sequence(entry->d_name)) {
      /* FIXME: parse file */
      count++;
	}
  }
  closedir(maildir);

  if(parse_sequence_file && count) {
    char *path = malloc(strlen(mbox->name) + strlen(".mh_sequences") + 2);
	sprintf(path, "%s/.mh_sequences", mbox->name);
    fp = fopen(path, "r");
    while(!feof(fp)) {
      /* FIXME: parse the contents */
    }
    fclose(fp);
    free(path);
  }

  stat(mbox->name, &st);
  data->mtime = st.st_mtime;

  if(msgs)
    *msgs = count;
  return 0;
}

/* 
 * Local atoi()
 * created this to guarantee that name is only digits, normal atoi allows
 * whitespace
 */
static int
mh_sequence(const char *name)
{
  char *sequence;
  int i;

  for(i = 0, sequence = name; *sequence; sequence++) {
    if(!isdigit(*sequence))
      return 0;
    i *= 10;
    i += (*sequence - '0');
  }
  return i;
}

/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH refile command */

#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

const char *argp_program_version = "refile (" PACKAGE_STRING ")";
static char doc[] = "GNU MH refile";
static char args_doc[] = N_("messages folder [folder...]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  'f', N_("FOLDER"), 0, N_("Specify folder to operate upon")},
  {"draft",   'd', NULL, 0, N_("Use <mh-dir>/draft as the source message")},
  {"link",    'l', N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("(not implemented) Preserve the source folder copy")},
  {"preserve", 'p', N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("(not implemented) Try to preserve message sequence numbers")},
  {"source", 's', N_("FOLDER"), 0,
   N_("Specify source folder. FOLDER will became the current folder after the program exits.")},
  {"src", 0, NULL, OPTION_ALIAS, NULL},
  {"file", 'F', N_("FILE"), 0, N_("Use FILE as the source message")},
  { N_("\nUse -help switch to obtain the list of traditional MH options. "), 0, 0, OPTION_DOC, "" },
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"file",    2, 'F', 0, "input-file"},
  {"draft",   1, 'd', 0, NULL },
  {"link",    1, 'l', MH_OPT_BOOL, NULL },
  {"preserve", 1, 'p', MH_OPT_BOOL, NULL },
  {"src",     1,  's', 0, "+folder" },
  { 0 }
};

int link_flag = 1;
int preserve_flag = 0;
char *source_file = NULL;
list_t folder_name_list = NULL;
list_t folder_mbox_list = NULL;

void
add_folder (const char *folder)
{
  if (!folder_name_list && list_create (&folder_name_list))
    {
      mh_error (_("can't create folder list"));
      exit (1);
    }
  list_append (folder_name_list, strdup (folder));
}

void
open_folders ()
{
  iterator_t itr;

  if (!folder_name_list)
    {
      mh_error (_("no folder specified"));
      exit (1);
    }

  if (list_create (&folder_mbox_list))
    {
      mh_error (_("can't create folder list"));
      exit (1);
    }

  if (iterator_create (&itr, folder_name_list))
    {
      mh_error (_("can't create iterator"));
      exit (1);
    }

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *name = NULL;
      mailbox_t mbox;
      
      iterator_current (itr, (void **)&name);
      mbox = mh_open_folder (name, 1);
      list_append (folder_mbox_list, mbox);
      free (name);
    }
  iterator_destroy (&itr);
  list_destroy (&folder_name_list);
}

void
enumerate_folders (void (*f) __P((void *, mailbox_t)), void *data)
{
  iterator_t itr;

  if (iterator_create (&itr, folder_mbox_list))
    {
      mh_error (_("can't create iterator"));
      exit (1);
    }

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      mailbox_t mbox;
      iterator_current (itr, (void **)&mbox);
      (*f) (data, mbox);
    }
  iterator_destroy (&itr);
}
  

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case '+':
    case 'f': 
      add_folder (arg);
      break;

    case 'd':
      source_file = mh_expand_name (NULL, "draft", 0);
      break;

    case 'l':
      link_flag = is_true(arg);
      break;
      
    case 'p':
      preserve_flag = is_true(arg);
      break;
	
    case 's':
      current_folder = arg;
      break;
      
    case 'F':
      source_file = arg;
      break;
      
    default:
      return 1;
    }
  return 0;
}

void
_close_folder (void *unused, mailbox_t mbox)
{
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
}

void
refile_folder (void *data, mailbox_t mbox)
{
  message_t msg = data;
  int rc;
  
  rc = mailbox_append_message (mbox, msg);
  if (rc)
    {
      mh_error (_("error appending message: %s"), mu_errstring (rc));
      exit (1);
    }
}

void
refile (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  enumerate_folders (refile_folder, msg);
  if (!link_flag)
    {
      attribute_t attr;
      message_get_attribute (msg, &attr);
      attribute_set_deleted (attr);
    }
}

mailbox_t
open_source (char *file_name)
{
  struct stat st;
  char *buffer;
  int fd;
  size_t len = 0;
  mailbox_t tmp;
  stream_t stream;
  char *p;
  
  if (stat (file_name, &st) < 0)
    {
      mh_error (_("can't stat file %s: %s"), file_name, strerror (errno));
      return NULL;
    }

  buffer = xmalloc (st.st_size+1);
  fd = open (file_name, O_RDONLY);
  if (fd == -1)
    {
      mh_error (_("can't open file %s: %s"), file_name, strerror (errno));
      return NULL;
    }

  if (read (fd, buffer, st.st_size) != st.st_size)
    {
      mh_error (_("error reading file %s: %s"), file_name, strerror (errno));
      return NULL;
    }

  buffer[st.st_size] = 0;
  close (fd);

  if (mailbox_create (&tmp, "/dev/null")
      || mailbox_open (tmp, MU_STREAM_READ) != 0)
    {
      mh_error (_("can't create temporary mailbox"));
      return NULL;
    }

  if (memory_stream_create (&stream, 0, MU_STREAM_RDWR)
      || stream_open (stream))
    {
      mailbox_close (tmp);
      mh_error (_("can't create temporary stream"));
      return NULL;
    }

  for (p = buffer; *p && isspace (*p); p++)
    ;

  if (strncmp (p, "From ", 5))
    {
      struct tm *tm;
      time_t t;
      char date[80];
      
      time(&t);
      tm = gmtime(&t);
      strftime (date, sizeof (date),
		"From GNU-MH-refile %a %b %e %H:%M:%S %Y%n",
		tm);
      stream_write (stream, date, strlen (date), 0, &len);
    }      

  stream_write (stream, p, strlen (p), len, &len);
  mailbox_set_stream (tmp, stream);
  if (mailbox_messages_count (tmp, &len)
      || len < 1)
    {
      mh_error (_("input file %s is not a valid message file"), file_name);
      return NULL;
    }
  else if (len > 1)
    {
      mh_error (_("input file %s contains %lu messages"),
		(unsigned long) len);
      return NULL;
    }
  free (buffer);
  return tmp;
}

int
main (int argc, char **argv)
{
  int index;
  mh_msgset_t msgset;
  mailbox_t mbox;
  int status;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  if (source_file)
    {
      if (index < argc)
	{
	  mh_error (_("both message set and source file given"));
	  exit (1);
	}
      mbox = open_source (source_file);
      mh_msgset_parse (mbox, &msgset, 0, NULL, "first");
    }
  else
    {
      mbox = mh_open_folder (current_folder, 0);
      mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "cur");
    }
  
  open_folders ();

  status = mh_iterate (mbox, &msgset, refile, NULL);
 
  enumerate_folders (_close_folder, NULL);
  mailbox_expunge (mbox);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return status;
}

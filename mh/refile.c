/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH refile command */

#include <mh.h>

const char *argp_program_version = "refile (" PACKAGE_STRING ")";
static char doc[] = "GNU MH refile";
static char args_doc[] = "messages folder [folder...]";

/* GNU options */
static struct argp_option options[] = {
  {"folder",  'f', "FOLDER", 0, "Specify folder to operate upon"},
  {"draft",   'd', NULL, 0, "Use <mh-dir>/draft as the source message"},
  {"link",    'l', "BOOL", OPTION_ARG_OPTIONAL, "Preserve the source folder copy"},
  {"preserve", 'p', "BOOL", OPTION_ARG_OPTIONAL, "Do not preserve the source folder copy (default)"},
  {"source", 's', "FOLDER", 0, "Specify source folder. FOLDER will became the current folder after the program exits."},
  {"src", 0, NULL, OPTION_ALIAS, NULL},
  {"file", 'F', "FILE", 0, "Use FILE as the source message"},
  { "\nUse -help switch to obtain the list of traditional MH options. ", 0, 0, OPTION_DOC, "" },
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
list_t out_folder_list = NULL;

void
add_folder (const char *folder)
{
  char *p;
  
  if (!out_folder_list && list_create (&out_folder_list))
    {
      mh_error ("can't create folder list");
      exit (1);
    }
  if ((p = strdup (folder)) == NULL)
    {
      mh_error("not enough memory");
      exit (1);
    }
  list_append (out_folder_list, p);
}

void
enumerate_folders (void (*f) __P((void *, char *)), void *data)
{
  iterator_t itr;
  int rc = 0;

  if (iterator_create (&itr, out_folder_list))
    {
      mh_error ("can't create iterator");
      exit (1);
    }

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *name;
      iterator_current (itr, (void **)&name);
      (*f) (data, name);
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
      source_file = "draft";
      break;

    case 'l':
      link_flag = arg[0] == 'y';
      break;
      
    case 'p':
      preserve_flag = arg[0] == 'y';
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


int
main (int argc, char **argv)
{
  int i, index;
  mh_msgset_t msgset;
  mailbox_t mbox;
  int status;
  
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (current_folder);
  mh_msgset_parse (mbox, &msgset, argc - index, argv + index);
  //  status = refile (mbox, &msgset);
  mailbox_expunge (mbox);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return status;
}

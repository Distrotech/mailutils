/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* MH folder command */

#include <mh.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <dirent.h>

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include <obstack.h>

const char *argp_program_version = "folder (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH folder\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[action] [msg]");

static struct argp_option options[] = {
  {N_("Actions are:"), 0, 0, OPTION_DOC, NULL, 0 },
  {"print", ARG_PRINT, NULL, 0,
   N_("List the folders (default)"), 1 },
  {"list",  ARG_LIST,  NULL, 0,
   N_("List the contents of the folder stack"), 1},
  {"push",  ARG_PUSH,  N_("FOLDER"), OPTION_ARG_OPTIONAL,
    N_("Push the folder on the folder stack. If FOLDER is specified, it is pushed. "
       "Otherwise, if a folder is given in the command line (via + or --folder), "
       "it is pushed on stack. Otherwise, the current folder and the top of the folder "
       "stack are exchanged"), 1},
  {"pop",   ARG_POP,    NULL, 0,
   N_("Pop the folder off the folder stack"), 1},
  
  {N_("Options are:"), 0, 0, OPTION_DOC, NULL, 2 },
  
  {"folder", ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify folder to operate upon"), 3},
  {"all",    ARG_ALL,    NULL, 0,
   N_("List all folders"), 3},
  {"create", ARG_CREATE, N_("BOOL"), OPTION_ARG_OPTIONAL, 
    N_("Create non-existing folders"), 3},
  {"fast",   ARG_FAST, N_("BOOL"), OPTION_ARG_OPTIONAL, 
    N_("List only the folder names"), 3},
  {"header", ARG_HEADER, N_("BOOL"), OPTION_ARG_OPTIONAL, 
    N_("Print the header line"), 3},
  {"recurse",ARG_RECURSIVE, N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("Scan folders recursively"), 3},
  {"total",  ARG_TOTAL, N_("BOOL"), OPTION_ARG_OPTIONAL, 
    N_("Output the total statistics"), 3},
  
  {NULL},
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"print",   2, 0, NULL },
  {"list",    1, 0, NULL },
  {"push",    2, 0, NULL },
  {"pop",     2, 0, NULL },
  {"all",     1, 0, NULL },
  {"create",  1, MH_OPT_BOOL, NULL},
  {"fast",    1, MH_OPT_BOOL, NULL},
  {"header",  1, MH_OPT_BOOL, NULL},
  {"recurse", 1, MH_OPT_BOOL, NULL},
  {"total",   1, MH_OPT_BOOL, NULL},
  {NULL},
};

typedef int (*folder_action) ();

static int action_print ();
static int action_list ();
static int action_push ();
static int action_pop ();

static folder_action action = action_print;

int show_all = 0; /* List all folders. Raised by --all switch */
int create_flag = 0; /* Create non-existent folders (--create) */
int fast_mode = 0; /* Fast operation mode. (--fast) */
int print_header = 0; /* Display the header line (--header) */
int recurse = 0; /* Recurse sub-folders */
int print_total; /* Display total stats */

char *push_folder; /* Folder name to push on stack */

char *mh_seq_name; /* Name of the mh sequence file (defaults to
		      .mh_sequences) */

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_PRINT:
      action = action_print;
      break;
      
    case ARG_LIST:
      action = action_list;
      break;
      
    case ARG_PUSH:
      action = action_push;
      if (arg)
	{
	  push_folder = mh_current_folder ();
	  current_folder = arg;
	}
      break;
      
    case ARG_POP:
      action = action_pop;
      break;
      
    case ARG_ALL:
      show_all++;
      break;

    case ARG_CREATE:
      create_flag = is_true (arg);
      break;

    case ARG_FAST:
      fast_mode = is_true (arg);
      break;

    case ARG_HEADER:
      print_header = is_true (arg);
      break;

    case ARG_RECURSIVE:
      recurse = is_true (arg);
      break;

    case ARG_TOTAL:
      print_total = is_true (arg);
      break;
      
    case ARG_FOLDER:
      push_folder = mh_current_folder ();
      current_folder = arg;
      break;
      
    default:
      return 1;
    }
  return 0;
}

struct folder_info {
  char *name;              /* Folder name */
  size_t message_count;    /* Number of messages in this folder */
  size_t min;              /* First used sequence number (=uid) */
  size_t max;              /* Last used sequence number */
  size_t cur;              /* UID of the current message */
  size_t others;           /* Number of non-message files */ 
};

struct obstack folder_info_stack; /* Memory storage for folder infp */
struct folder_info *folder_info;  /* Finalized array of information
				     structures */
size_t folder_info_count;         /* Number of the entries in the array */

size_t message_count;             /* Total number of messages */

int name_prefix_len;              /* Length of the mu_path_folder_dir */


void
install_folder_info (const char *name, struct folder_info *info)
{
  info->name = strdup (name) + name_prefix_len;
  obstack_grow (&folder_info_stack, info, sizeof (*info));
  folder_info_count++;
  message_count += info->message_count;
}

static int
folder_info_comp (const void *a, const void *b)
{
  return strcmp (((struct folder_info *)a)->name,
		 ((struct folder_info *)b)->name);
}

static void
read_seq_file (struct folder_info *info, const char *prefix, const char *name)
{
  char *pname = NULL;
  mh_context_t *ctx;
  char *p;
  
  asprintf (&pname, "%s/%s", prefix, name);
  if (!pname)
    abort ();
  ctx = mh_context_create (pname, 1);
  mh_context_read (ctx);
  
  p = mh_context_get_value (ctx, "cur", NULL);
  if (p)
    info->cur = strtoul (p, NULL, 0);
  free (pname);
  free (ctx);
}

static void
_scan (const char *name, int depth)
{
  DIR *dir;
  struct dirent *entry;
  struct folder_info info;
  char *p;
  struct stat st;
  size_t uid;

  if (!recurse)
    {
      if (fast_mode && depth > 0)
	{
	  memset (&info, 0, sizeof (info));
	  info.name = strdup (name);
	  install_folder_info (name, &info);
	  return;
	}
      
      if (depth > 1)
	return;
    }
  
  dir = opendir (name);

  if (!dir && errno == ENOENT && create_flag)
    {
      mh_check_folder (name, 0);
      dir = opendir (name);
    }

  if (!dir)
    {
      mh_error (_("can't scan folder %s: %s"), name, strerror (errno));
      return;
    }

  memset (&info, 0, sizeof (info));
  info.name = strdup (name);
  while ((entry = readdir (dir)))
    {
      if (entry->d_name[0] == '.')
	{
	  if (strcmp (entry->d_name, mh_seq_name) == 0)
	    read_seq_file (&info, name, entry->d_name);
	}
      else if (entry->d_name[0] != ',')
	{
	  asprintf (&p, "%s/%s", name, entry->d_name);
	  if (stat (p, &st) < 0)
	    mh_error (_("can't stat %s: %s"), p, strerror (errno));
	  else if (S_ISDIR (st.st_mode))
	    {
	      info.others++;
	      _scan (p, depth+1);
	    }
	  else
	    {
	      char *endp;
	      uid = strtoul (entry->d_name, &endp, 10);
	      if (*endp)
		info.others++;
	      else
		{
		  info.message_count++;
		  if (info.min == 0 || uid < info.min)
		    info.min = uid;
		  if (uid > info.max)
		    info.max = uid;
		}
	    }
	}
    }
  
  if (info.cur)
    {
      asprintf (&p, "%s/%lu", name, (unsigned long) info.cur);
      if (stat (p, &st) < 0 || !S_ISREG (st.st_mode))
	info.cur = 0;
      free (p);
    }
  closedir (dir);
  if (depth > 0)
    install_folder_info (name, &info);
}
    
static void
print_all ()
{
  struct folder_info *info, *end = folder_info + folder_info_count;

  for (info = folder_info; info < end; info++)
    {
      int len = strlen (info->name);
      if (len < 22)
	printf ("%22.22s", info->name);
      else
	printf ("%s", info->name);
      
      if (strcmp (info->name, current_folder) == 0)
	printf ("+");
      else
	printf (" ");
      
      if (info->message_count)
	{
	  printf (ngettext(" has %4lu message  (%4lu-%4lu)",
			   " has %4lu messages (%4lu-%4lu)",
			   info->message_count),
		  (unsigned long) info->message_count,
		  (unsigned long) info->min,
		  (unsigned long) info->max);
	  if (info->cur)
	    printf ("; cur=%4lu", (unsigned long) info->cur);
	}
      else
	{
	  printf (_(" has no messages"));
	}
      
      if (info->others)
	{
	  if (!info->cur)
	    printf (";           ");
	  else
	    printf ("; ");
	  printf (_("(others)"));
	}
      printf (".\n");
    }
}

static void
print_fast ()
{
  struct folder_info *info, *end = folder_info + folder_info_count;

  for (info = folder_info; info < end; info++)
    printf ("%s\n", info->name);
}

static int
action_print ()
{
  mh_seq_name = mh_global_profile_get ("mh-sequences", MH_SEQUENCES_FILE);

  name_prefix_len = strlen (mu_path_folder_dir);
  if (mu_path_folder_dir[name_prefix_len - 1] == '/')
    name_prefix_len++;
  name_prefix_len++;  /* skip past the slash */

  obstack_init (&folder_info_stack);

  if (show_all)
    {
      _scan (mu_path_folder_dir, 0);
    }
  else
    {
      char *p = mh_expand_name (NULL, current_folder, 0);
      _scan (p, 1);
      free (p);
    }
  
  folder_info = obstack_finish (&folder_info_stack);
  qsort (folder_info, folder_info_count, sizeof (folder_info[0]),
	 folder_info_comp);

  if (fast_mode)
    print_fast ();
  else
    {
      if (print_header)                                   
	printf (_("Folder                  # of messages     (  range  )  cur msg   (other files)\n"));
		
      print_all ();

      if (print_total)
	{
	  printf ("\n%24.24s=", _("TOTAL"));
	  printf (ngettext ("%4lu message  ", "%4lu messages ",
			    message_count),
		  (unsigned long) message_count);
	  printf (ngettext ("in %4lu folder", "in %4lu folders",
			    folder_info_count),
		  (unsigned long) folder_info_count);
	  printf ("\n");
	}
    }
  if (push_folder)
    mh_global_save_state ();

  return 0;
}

static int
action_list ()
{
  char *stack = mh_global_context_get ("Folder-Stack", NULL);

  printf ("%s", current_folder);
  if (stack)
    printf (" %s", stack);
  printf ("\n");
  return 0;
}

static char *
make_stack (char *folder, char *old_stack)
{
  char *stack = NULL;
  if (old_stack)
    asprintf (&stack,  "%s %s", folder, old_stack);
  else
    stack = strdup (folder);
  return stack;
}

static int
action_push ()
{
  char *stack = mh_global_context_get ("Folder-Stack", NULL);
  char *new_stack = NULL;

  if (push_folder)
    new_stack = make_stack (push_folder, stack);
  else
    {
      char *s, *p = strtok_r (stack, " ", &s);
      new_stack = make_stack (current_folder, stack);
      current_folder = p;
    }
  
  mh_global_context_set ("Folder-Stack", new_stack);
  action_list ();
  mh_global_save_state ();
  return 0;
}

static int
action_pop ()
{
  char *stack = mh_global_context_get ("Folder-Stack", NULL);
  char *s, *p;

  if (stack)
    {
      p = strtok_r (stack, " ", &s);
      if (s[0] == 0)
	s = NULL;
    }
  else
    {
      p = current_folder;
      s = NULL;
    }
  mh_global_context_set ("Folder-Stack", s);
  current_folder = p;
  action_list ();
  mh_global_save_state ();
  return 0;
}

int
main (int argc, char **argv)
{
  int index = 0;
  mh_msgset_t msgset;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  /* If  folder  is invoked by a name ending with "s" (e.g.,  folders),
     `-all'  is  assumed */
  if (program_invocation_short_name[strlen (program_invocation_short_name) - 1] == 's')
    show_all++;

  if (argc - index == 1)
    {
      mailbox_t mbox = mh_open_folder (current_folder, 0);
      mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "cur");
      mh_msgset_current (mbox, &msgset, 0);
      mh_global_save_state ();
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
    }
  else if (argc - index > 1)
    {
      mh_error (_("too many arguments"));
      exit (1);
    }
  
  if (show_all)
    print_header = print_total = 1;
  
  return (*action) ();
}

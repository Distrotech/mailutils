/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2012, 2014-2016 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

/* MH scan command */

#include <mh.h>
#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <mailutils/observer.h>

static char progdoc[] = N_("GNU MH scan");
static char args_doc[] = N_("[+FOLDER] [MSGLIST]");

static int clear;
static char *format_str = mh_list_format;

static int width = 80;
static int reverse;
static int header;

static mh_format_t format;

static mu_msgset_t msgset;

static void
form_handler (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (mh_read_formfile (arg, &format_str))
    exit (1);
}

static struct mu_option options[] = {
  { "clear",   0, NULL, MU_OPTION_DEFAULT,
    N_("clear screen after displaying the list"),
    mu_c_bool, &clear },
  { "form",    0, N_("FILE"),   MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, NULL, form_handler },
  { "format",  0, N_("FORMAT"), MU_OPTION_DEFAULT,
    N_("use this format string"),
    mu_c_string, &format_str },
  { "header",  0, NULL,   MU_OPTION_DEFAULT,
    N_("display header"),
    mu_c_bool, &header },
  { "width",   0, N_("NUMBER"), MU_OPTION_DEFAULT,
    N_("set output width"),
    mu_c_int, &width },
  { "reverse",  0, NULL,   MU_OPTION_DEFAULT,
    N_("list messages in reverse order"),
    mu_c_bool, &reverse },
  { "file",     0, N_("FILE"),   MU_OPTION_HIDDEN,
    N_("[not yet implemented]"),
    mu_c_string, NULL, mh_opt_notimpl },
  MU_OPTION_END
};

static int list_message (size_t num, mu_message_t msg, void *data);
void print_header (mu_mailbox_t mbox);
void clear_screen (void);

/* Observable Action this is called at every message discover.  */
static int
action (mu_observer_t o, size_t type, void *data, void *action_data)
{
  static int counter;
  mu_mailbox_t mbox;
  mu_message_t msg = NULL;
  size_t num;

  if (type == MU_EVT_MESSAGE_ADD)
    {
      mbox = mu_observer_get_owner (o);
      counter++;
      mu_mailbox_get_message (mbox, counter, &msg);
      mh_message_number (msg, &num);
      list_message (num, msg, NULL);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox;
  int status;
  size_t total = 0;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, progdoc, NULL);

  if (mh_format_parse (format_str, &format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }

  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);

  if ((argc == 0 || strcmp (argv[0], "all") == 0) && !reverse)
    {
      /* Fast approach */
      mu_observer_t observer;
      mu_observable_t observable;
        
      print_header (mbox);
      
      mu_observer_create (&observer, mbox);
      mu_observer_set_action (observer, action, mbox);
      mu_mailbox_get_observable (mbox, &observable);
      mu_observable_attach (observable, MU_EVT_MESSAGE_ADD, observer);

      status = mu_mailbox_scan (mbox, 1, &total);
    }
  else
    {
      mu_mailbox_messages_count (mbox, &total);
      mh_msgset_parse (&msgset, mbox, argc, argv, "all");

      print_header (mbox);
      status = mu_msgset_foreach_dir_message (msgset, reverse,
					      list_message, NULL);
    }

  if (total == 0)
    {
      mu_url_t url = NULL;

      mu_mailbox_get_url (mbox, &url);
      mu_error (_("no messages in %s"), mu_url_to_string (url));
    }

  clear_screen ();
  mh_global_save_state ();
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return status;
}

void
print_header (mu_mailbox_t mbox)
{
  if (header)
    {
      mu_url_t url = NULL;
      char datestr[64];
      time_t t;
	  
      mu_mailbox_get_url (mbox, &url);
      time (&t);
      strftime (datestr, sizeof datestr, "%c", localtime (&t));
      printf (_("Folder %s  %s\n"), mu_url_to_string (url), datestr);
    }
}

#ifdef HAVE_TERMCAP_H
/* A subroutine for tputs */
int
putstdout(char c)
{
  return putc(c, stdout);
}
#endif

void
clear_screen ()
{
  if (clear)
    {
#ifdef HAVE_TERMCAP_H
      if (isatty (1))
	{
	  char termcap_buf[1024];
	  char *buffer = termcap_buf;
	  char *termname;
      
	  if ((termname = getenv("TERM")) == NULL)
	    /* No terminal; Try ansi */
	    termname = "ansi";
      
	  if (tgetent(termcap_buf, termname) == 1)
	    {
	      char *clr = tgetstr ("cl", &buffer);
	      if (clr)
		{
		  tputs(clr, 1, (int (*)())putstdout);
		  return;
		}
	    }
	}
#endif
      /* Fall back to formfeed */
      fprintf (stdout, "\f");
    }
}

static int
list_message (size_t num, mu_message_t msg, void *data)
{
  char *buffer;
  int len;
  
  mh_format (&format, msg, num, width, &buffer);
  printf ("%s", buffer);
  len = strlen (buffer);
  if (len > 0 && buffer[len-1] != '\n')
    printf("\n");
  free (buffer);
  return 0;
}

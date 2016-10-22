/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2012, 2014-2016 Free Software Foundation,
   Inc.

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

/* MH sortm command */

#include <mh.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

static char prog_doc[] = N_("Sort GNU MH messages");
static char args_doc[] = N_("[MSGLIST]");

static int limit;
static int verbose;
static mu_mailbox_t mbox;
static const char *mbox_path;

static size_t *msgarr;
static size_t msgcount;

static size_t current_num;

enum
  {
    ACTION_REORDER,
    ACTION_DRY_RUN,
    ACTION_LIST
  };

enum
  {
    algo_quicksort,
    algo_shell
  };
static int algorithm = algo_quicksort;
static int action = ACTION_REORDER;
static char *format_str = mh_list_format;
static mh_format_t format;

typedef int (*compfun) (void *, void *);
static void addop (char const *field, compfun comp);
static void remop (compfun comp);
static int comp_text (void *a, void *b);
static int comp_date (void *a, void *b);
static int comp_number (void *a, void *b);

static void
add_datefield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  addop (arg, comp_date);
}

static void
add_numfield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  addop (arg, comp_number);
}

static void
add_textfield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  addop (arg, comp_text);
}

static void
rem_datefield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  remop (comp_date);
}

static void
rem_numfield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  remop (comp_number);
}

static void
rem_textfield (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  remop (comp_text);
}

static void
set_action_reorder (struct mu_parseopt *po, struct mu_option *opt,
		    char const *arg)
{
  action = ACTION_REORDER;
}

static void
set_action_list (struct mu_parseopt *po, struct mu_option *opt,
		    char const *arg)
{
  action = ACTION_LIST;
}

static void
set_action_dry_run (struct mu_parseopt *po, struct mu_option *opt,
		    char const *arg)
{
  action = ACTION_DRY_RUN;
  if (!verbose)
    verbose = 1;
}

static void
set_algo_shell (struct mu_parseopt *po, struct mu_option *opt,
		char const *arg)
{
  algorithm = algo_shell;
}

static void
set_algo_quicksort (struct mu_parseopt *po, struct mu_option *opt,
		    char const *arg)
{
  algorithm = algo_quicksort;
}

static struct mu_option options[] = {
  MU_OPTION_GROUP (N_("Setting sort keys:")),
  { "datefield",     0, N_("STRING"), MU_OPTION_DEFAULT,
    N_("sort on the date field (default `Date:')"),
    mu_c_string, NULL, add_datefield },
  { "nodatefield",   0, NULL,       MU_OPTION_DEFAULT,
    N_("don't sort on the date field"),
    mu_c_string, NULL, rem_datefield },
  { "limit",         0, N_("DAYS"), MU_OPTION_DEFAULT,
    N_("consider two datefields equal if their difference lies within the given nuber of DAYS."),
    mu_c_int, &limit },
  { "nolimit",       0,       NULL,       MU_OPTION_DEFAULT,
    N_("undo the effect of the last -limit option"),
    mu_c_int, &limit, NULL, "-1" },
  { "textfield",     0,       N_("STRING"), MU_OPTION_DEFAULT,
    N_("sort on the text field"),
    mu_c_string, NULL, add_textfield },
  { "notextfield",   0,       NULL,       MU_OPTION_DEFAULT, 
    N_("don't sort on the text field"),
    mu_c_string, NULL, rem_textfield },
  { "numfield",      0,       N_("STRING"), MU_OPTION_DEFAULT,
    N_("sort on the numeric field"),
    mu_c_string, NULL, add_numfield },
  { "nonumfield",    0,       NULL,       MU_OPTION_DEFAULT,
    N_("don't sort on the numeric field"),
    mu_c_string, NULL, rem_numfield },
  
  MU_OPTION_GROUP (N_("Actions:")),
  { "reorder", 0,    NULL, MU_OPTION_DEFAULT,
    N_("reorder the messages (default)"),
    mu_c_string, NULL, set_action_reorder },
  { "dry-run", 0,    NULL, MU_OPTION_DEFAULT,
   N_("do not do anything, only show what would have been done"),
    mu_c_string, NULL, set_action_dry_run },
  { "list",    0,    NULL, MU_OPTION_DEFAULT,
    N_("list the sorted messages"), 
    mu_c_string, NULL, set_action_list },
  
  { "form",    0, N_("FILE"),   MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, &format_str, mh_opt_read_formfile },
  { "format",  0, N_("FORMAT"), MU_OPTION_DEFAULT,
    N_("use this format string"),
    mu_c_string, &format_str },

  { "verbose",  0, NULL,   MU_OPTION_DEFAULT,
    N_("verbosely list executed actions"),
    mu_c_bool, &verbose },

  MU_OPTION_GROUP (N_("Select sort algorithm:")),
  { "shell",    0, NULL,   MU_OPTION_DEFAULT,
    N_("use shell algorithm"),
    mu_c_string, NULL, set_algo_shell },
  
  { "quicksort", 0, NULL,   MU_OPTION_DEFAULT,
    N_("use quicksort algorithm (default)"),
    mu_c_string, NULL, set_algo_quicksort },

  MU_OPTION_END
};

/* *********************** Comparison functions **************************** */
struct comp_op
{
  char const *field;
  compfun comp;
};

static mu_list_t oplist;

static void
addop (char const *field, compfun comp)
{
  struct comp_op *op = mu_alloc (sizeof (*op));
  
  if (!oplist)
    {
      if (mu_list_create (&oplist))
	{
	  mu_error (_("can't create operation list"));
	  exit (1);
	}
      mu_list_set_destroy_item (oplist, mu_list_free_item);
    }
  op->field = field;
  op->comp = comp;
  mu_list_append (oplist, op);
}

struct rem_data
{
  struct comp_op *op;
  compfun comp;
};

static int
rem_action (void *item, void *data)
{
  struct comp_op *op = item;
  struct rem_data *d = data;
  if (d->comp == op->comp)
    d->op = op;
  return 0;
}

static void
remop (compfun comp)
{
  struct rem_data d;
  d.comp = comp;
  d.op = NULL;
  mu_list_foreach (oplist, rem_action, &d);
  mu_list_remove (oplist, d.op);
}

struct comp_data
{
  int r;
  mu_message_t m[2];
};

static int
compare_action (void *item, void *data)
{
  struct comp_op *op = item;
  struct comp_data *dp = data;
  char *a, *ap, *b, *bp;
  mu_header_t h;
  
  if (mu_message_get_header (dp->m[0], &h)
      || mu_header_aget_value (h, op->field, &a))
    return 0;

  if (mu_message_get_header (dp->m[1], &h)
      || mu_header_aget_value (h, op->field, &b))
    {
      free (a);
      return 0;
    }

  ap = a;
  bp = b;
  if (mu_c_strcasecmp (op->field, MU_HEADER_SUBJECT) == 0)
    {
      if (mu_c_strncasecmp (ap, "re:", 3) == 0)
	ap += 3;
      if (mu_c_strncasecmp (b, "re:", 3) == 0)
	bp += 3;
    }
  
  dp->r = op->comp (ap, bp);
  free (a);
  free (b);

  return dp->r; /* go on until the difference is found */
}

static int
compare_messages (mu_message_t a, mu_message_t b, size_t anum, size_t bnum)
{
  struct comp_data d;

  d.r = 0;
  d.m[0] = a;
  d.m[1] = b;
  mu_list_foreach (oplist, compare_action, &d);
  if (d.r == 0)
    {
      if (anum < bnum)
	d.r = -1;
      else if (anum > bnum)
	d.r = 1;
    }
      
  if (verbose > 1)
    fprintf (stderr, "%d\n", d.r);
  return d.r;
}

static int
comp_text (void *a, void *b)
{
  return mu_c_strcasecmp (a, b);
}

static int
comp_number (void *a, void *b)
{
  long na, nb;

  na = strtol (a, NULL, 0);
  nb = strtol (b, NULL, 0);
  if (na > nb)
    return 1;
  else if (na < nb)
    return -1;
  return 0;
}

/*FIXME: Also used in imap4d*/
static int
_parse_822_date (char *date, time_t * timep)
{
  struct tm tm;
  struct mu_timezone tz;
  const char *p = date;

  if (mu_parse822_date_time (&p, date + strlen (date), &tm, &tz) == 0)
    {
      *timep = mu_datetime_to_utc (&tm, &tz);
      return 0;
    }
  return 1;
}

static int
comp_date (void *a, void *b)
{
  time_t ta, tb;
  
  if (_parse_822_date (a, &ta) || _parse_822_date (b, &tb))
    return 0;

  if (ta < tb)
    {
      if (limit && tb - ta <= limit)
	return 0;
      return -1;
    }
  else if (ta > tb)
    {
      if (limit && ta - tb <= limit)
	return 0;
      return 1;
    }
  return 0;
}


/* *********************** Sorting routines ***************************** */

static int
comp0 (size_t na, size_t nb)
{
  mu_message_t a, b;

  if (mu_mailbox_get_message (mbox, na, &a)
      || mu_mailbox_get_message (mbox, nb, &b))
    return 0;
  if (verbose > 1)
    fprintf (stderr,
	     _("comparing messages %s and %s: "),
	     mu_umaxtostr (0, na),
	     mu_umaxtostr (1, nb));
  return compare_messages (a, b, na, nb);
}

int
comp (const void *a, const void *b)
{
  return comp0 (* (size_t*) a, * (size_t*) b);
}


/* ****************************** Shell sort ****************************** */
#define prevdst(h) ((h)-1)/3

static int
startdst (unsigned count, int *num)
{
  int i, h;

  for (i = h = 1; 9*h + 4 < count; i++, h = 3*h+1)
    ;
  *num = i;
  return h;
}

void
shell_sort ()
{
    int h, s, i, j;
    size_t hold;

    for (h = startdst (msgcount, &s); s > 0; s--, h = prevdst (h))
      {
	if (verbose > 1)
	  fprintf (stderr, _("distance %d\n"), h);
        for (j = h; j < msgcount; j++)
	  {
            hold = msgarr[j];
            for (i = j - h;
		 i >= 0 && comp0 (hold, msgarr[i]) < 0; i -= h)
	      msgarr[i + h] = msgarr[i];
	    msgarr[i + h] = hold;
	  }
      }
}


/* ****************************** Actions ********************************* */

void
list_message (size_t num)
{
  mu_message_t msg = NULL;
  char *buffer;
  mu_mailbox_get_message (mbox, num, &msg);
  mh_format (&format, msg, num, 76, &buffer);
  printf ("%s\n", buffer);
  free (buffer);
}

void
swap_message (size_t a, size_t b)
{
  char *path_a, *path_b;
  char *tmp;
  
  path_a = mh_safe_make_file_name (mbox_path, mu_umaxtostr (0, a));
  path_b = mh_safe_make_file_name (mbox_path, mu_umaxtostr (1, b));
  tmp = mu_tempname (mbox_path);
  rename (path_a, tmp);
  unlink (path_a);
  rename (path_b, path_a);
  unlink (path_b);
  rename (tmp, path_b);
  free (tmp);
}

void
transpose(size_t i, size_t n)
{
  size_t j;

  for (j = i+1; j < msgcount; j++)
    if (msgarr[j] == n)
      {
	size_t t = msgarr[i];
	msgarr[i] = msgarr[j];
	msgarr[j] = t;
	break;
      }
}
  
static int got_signal;

RETSIGTYPE
sighandler (int sig)
{
  got_signal = 1;
}

void
sort ()
{
  size_t *oldlist, i;
  oldlist = mu_alloc (msgcount * sizeof (*oldlist));
  memcpy (oldlist, msgarr, msgcount * sizeof (*oldlist));

  switch (algorithm)
    {
    case algo_quicksort:
      qsort (msgarr, msgcount, sizeof (msgarr[0]), comp);
      break;

    case algo_shell:
      shell_sort ();
      break;
    }

  switch (action)
    {
    case ACTION_LIST:
      for (i = 0; i < msgcount; i++)
	list_message (msgarr[i]);
      break;

    default:
      /* Install signal handlers */
      signal (SIGINT, sighandler);
      signal (SIGQUIT, sighandler);
      signal (SIGTERM, sighandler);
      
      if (verbose)
	fprintf (stderr, _("Transpositions:\n"));
      for (i = 0, got_signal = 0; !got_signal && i < msgcount; i++)
	{
	  if (msgarr[i] != oldlist[i])
	    {
	      size_t old_num, new_num;
	      mu_message_t msg;

	      mu_mailbox_get_message (mbox, oldlist[i], &msg);
	      mh_message_number (msg, &old_num);
	      mu_mailbox_get_message (mbox, msgarr[i], &msg);
	      mh_message_number (msg, &new_num);
	      transpose (i, oldlist[i]);
	      if (verbose)
		{
		  fprintf (stderr, "{%s, %s}",
			   mu_umaxtostr (0, old_num),
			   mu_umaxtostr (1, new_num));
		}
	      if (old_num == current_num)
		{
		  if (verbose)
		    fputc ('*', stderr);
		  current_num = new_num;
		}
	      else if (new_num == current_num)
		{
		  if (verbose)
		    fputc ('*', stderr);
		  current_num = old_num;
		}
	      if (verbose)
		fputc ('\n', stderr);
	      if (action == ACTION_REORDER)
		swap_message (old_num, new_num);
	    }
	}
    }
  if (action == ACTION_REORDER)
    {
      mu_mailbox_close (mbox);
      mu_mailbox_open (mbox, MU_STREAM_RDWR);
      mh_mailbox_set_cur (mbox, current_num);
    }
}

static int
_add_msgno (size_t n, void *data)
{
  size_t *pidx = data;
  msgarr[*pidx] = n;
  ++*pidx;
  return 0;
}

static void
fill_msgarr (mu_msgset_t msgset)
{
  size_t i;
  int rc;
  
  rc = mu_msgset_count (msgset, &msgcount);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_count", NULL, rc);
      exit (1);
    }
      
  msgarr = mu_calloc (msgcount, sizeof (msgarr[0]));
  i = 0;
  rc = mu_msgset_foreach_msgno (msgset, _add_msgno, &i);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_foreach_msgno", NULL, rc);
      exit (1);
    }
}


int
main (int argc, char **argv)
{
  mu_url_t url;
  mu_msgset_t msgset;
  
  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);
  
  if (!oplist)
    addop ("date", comp_date);

  if (action == ACTION_LIST && mh_format_parse (format_str, &format))
    {
      mu_error (_("Bad format string"));
      exit (1);
    }
  
  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);
  mu_mailbox_get_url (mbox, &url);
  mbox_path = mu_url_to_string (url);
  if (memcmp (mbox_path, "mh:", 3) == 0)
    mbox_path += 3;
  
  mh_mailbox_get_cur (mbox, &current_num);

  mh_msgset_parse (&msgset, mbox, argc, argv, "all");
  fill_msgarr (msgset);
  mu_msgset_free (msgset);
  sort ();
  mh_global_save_state ();
  mu_mailbox_destroy (&mbox);
  
  return 0;
}

#include "config.h"
#include <mailutils/mailutils.h>
#include <stdio.h>
#include <argp.h>

const char *argp_program_version = "messages (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU messages -- count the number of messages in a mailbox";
static char args_doc[] = "[mailbox...]";

static struct argp_option options[] = {
  { 0 }
};

struct arguments
{
  char **args;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int
main (int argc, char **argv)
{
  int i = 1;
  list_t bookie;
  mailbox_t mbox;
  int count;
  int err = 0;
  struct arguments args;
  args.args = NULL;

  argp_parse (&argp, argc, argv, 0, 0, &args);

  registrar_get_list (&bookie);
  list_append (bookie, path_record);

  /* FIXME: if argc < 2, check on $MAIL and exit */

  for (i=1; i < argc; i++)
    {
      if (mailbox_create_default (&mbox, argv[i]) != 0)
	{
	  fprintf (stderr, "Couldn't create mailbox %s.\n", argv[i]);
	  err = 1;
	  continue;
	}
      if (mailbox_open (mbox, MU_STREAM_READ) != 0)
	{
	  fprintf (stderr, "Couldn't open mailbox %s.\n", argv[i]);
	  err = 1;
	  continue;
	}
      if (mailbox_messages_count (mbox, &count) != 0)
	{
	  fprintf (stderr, "Couldn't count messages in %s.\n", argv[i]);
	  err = 1;
	  continue;
	}

      printf ("Number of messages in %s: %d\n", argv[i], count);

      if (mailbox_close (mbox) != 0)
	{
	  fprintf (stderr, "Couldn't close %s.\n", argv[i]);
	  err = 1;
	  continue;
	}
      mailbox_destroy (&mbox);
    }
  return 0;
}

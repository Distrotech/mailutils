#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/errno.h>
#include <mailutils/mailbox.h>
#include <mailutils/parse822.h>
#include <mailutils/registrar.h>

int
main (int argc, char **argv)
{
  mailbox_t mbox;
  size_t count = 0;
  char *mboxname = argv[1];
  int status;

  /* Register desired mailbox formats. */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
    list_append (bookie, imap_record);
  }

  if ((status = mailbox_create_default (&mbox, mboxname)) != 0)
    {
      fprintf (stderr, "could not create <%s>: %s\n",
	       mboxname, mu_errstring (status));
      return status;
    }

  {
    mu_debug_t debug;
    mailbox_get_debug (mbox, &debug);
    mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);
  }

  if ((status = mailbox_open (mbox, MU_STREAM_READ)) != 0)
    {
      fprintf (stderr, "could not open <%s>: %s\n",
	  mboxname, mu_errstring (status));
      mailbox_destroy (&mbox);
      exit (1);
    }

  if ((status = mailbox_messages_count (mbox, &count)) != 0)
    {
      fprintf (stderr, "could not count  <%s>: %s\n",
	  mboxname, mu_errstring (status));
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
      exit (1);
    }

  mailbox_close (mbox);
  mailbox_destroy (&mbox);

  printf("count %d messages in <%s>\n", count, mboxname);

  return 0;
}


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

#include <mailutils/mailbox.h>
#include <mailutils/address.h>
#include <mailutils/registrar.h>
#include <mailutils/parse822.h>


static const char *
UserAgent (header_t hdr)
{
  static char agent[128];
  size_t sz;

  if (header_get_value (hdr, "User-Agent", agent, sizeof (agent), &sz) == 0
      && sz != 0)
    return agent;

  if (header_get_value (hdr, "X-Mailer", agent, sizeof (agent), &sz) == 0
      && sz != 0)
    return agent;

  if (header_get_value (hdr, "Message-Id", agent, sizeof (agent), &sz) == 0
      && sz != 0)
    return agent;

  return "unknown";
}

int
main (int argc, char **argv)
{
  mailbox_t mbox;
  size_t i;
  size_t count = 0;
  char *mboxname = argv[1];
  int status;

  /* Register desired mailbox formats. */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, path_record);
  }

  if ((status = mailbox_create_default (&mbox, mboxname)) != 0)
    {
      fprintf (stderr, "could not create <%s>: %s\n",
	       mboxname, strerror (status));
      exit (1);
    }

  {
    debug_t debug;
    mailbox_get_debug (mbox, &debug);
//  debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
  }

  if ((status = mailbox_open (mbox, MU_STREAM_READ)) != 0)
    {
      fprintf (stderr, "could not open mbox: %s\n", strerror (status));
      exit (1);
    }

  mailbox_messages_count (mbox, &count);

  for (i = 1; i <= count; ++i)
    {
      message_t msg;
      header_t hdr;
      char date[128];
      size_t len = 0;

      if ((status = mailbox_get_message (mbox, i, &msg)) != 0 ||
	  (status = message_get_header (msg, &hdr)) != 0)
	{
	  printf ("%s, msg %d: %s\n", mboxname, i, strerror (status));
	  continue;
	}
      if ((status =
	   header_get_value (hdr, MU_HEADER_DATE, date, sizeof (date),
			     &len)) != 0)
	{
	  printf ("%s, msg %d: NO DATE (mua? %s)\n",
		  mboxname, i, UserAgent (hdr));
	  continue;
	}
      else
	{
	  const char *s = date;

	  if (parse822_date_time (&s, s + strlen (s), NULL, NULL))
	    {
	      printf ("%s, msg %d: BAD DATE <%s> (mua? %s)\n",
		      mboxname, i, date, UserAgent (hdr));
	      continue;
	    }
	}
    }

  mailbox_close (mbox);
  mailbox_destroy (&mbox);

  return status;
}

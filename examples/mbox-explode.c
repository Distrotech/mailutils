#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mailutils/mailbox.h>
#include <mailutils/registrar.h>

int
main (int argc, char **argv)
{
  mailbox_t mbox;
  size_t msgno;
  size_t count = 0;
  char *mbox_name = 0;
  char *dir_name = 0;
  int status;

  if (argc != 3)
    {
      printf ("usage: mbox-explode <mbox> <directory>\n");
      exit (0);
    }
  mbox_name = argv[1];
  dir_name = argv[2];

  if (mkdir (dir_name, 0777) != 0)
    {
      fprintf (stderr, "mkdir(%s) failed: %s\n", dir_name, strerror (errno));
      exit (1);
    }
  /* Register the desire formats.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, path_record);
  }

  if ((status = mailbox_create_default (&mbox, mbox_name)) != 0)
    {
      fprintf (stderr, "could not create <%s>: %s\n",
	       mbox_name, strerror (status));
      exit (1);
    }
  {
    debug_t debug;
    mailbox_get_debug (mbox, &debug);
    debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);
  }

  if ((status = mailbox_open (mbox, MU_STREAM_READ)) != 0)
    {
      fprintf (stderr, "could not open <%s>: %s\n",
	       mbox_name, strerror (status));
      exit (1);
    }
  mailbox_messages_count (mbox, &count);

  for (msgno = 1; msgno <= count; ++msgno)
    {
      message_t msg = 0;
      size_t nparts = 0;
      size_t partno;

      if ((status = mailbox_get_message (mbox, msgno, &msg)) != 0)
	{
	  fprintf (stderr, "msg %d: get message failed: %s\n",
		   msgno, strerror (status));
	  exit (2);
	}
      if ((status = message_get_num_parts (msg, &nparts)))
	{
	  fprintf (stderr, "msg %d: get num parts failed: %s\n",
		   msgno, strerror (status));
	  exit (1);
	}
      printf ("msg %03d: %02d parts\n", msgno, nparts);

      for (partno = 1; partno <= nparts; partno++)
	{
	  message_t part = 0;
	  char path[128];

	  sprintf (path, "%s/m%03d.p%02d", dir_name, msgno, partno);

	  printf ("msg %03d: part %02d > %s\n", msgno, partno, path);

	  if ((status = message_get_part (msg, partno, &part)))
	    {
	      fprintf (stderr, "msg %d: get part %d failed: %s\n",
		       msgno, partno, strerror (status));
	      exit (1);
	    }
	  if ((status = message_save_attachment (part, path, 0)))
	    {
	      fprintf (stderr, "msg %d part %d: save failed: %s\n",
		       msgno, partno, strerror (status));
	      break;
	    }
	}
    }

  mailbox_close (mbox);
  mailbox_destroy (&mbox);

  return status;
}

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

int
main(int argc, char **argv)
{
  mailbox_t mbox;
  size_t i;
  size_t count = 0;
  char *mailbox_name;
  int status;

  mailbox_name = (argc > 1) ? argv[0] : "+dbuild.details";

  /* Register the desire formats.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, path_record);
  }

  if ((status = mailbox_create_default (&mbox, mailbox_name)) != 0)
  {
    fprintf (stderr, "could not create <%s>: %s\n",
      mailbox_name, strerror (status));
    exit (1);
  }

  {
    debug_t debug;
    mailbox_get_debug (mbox, &debug);
//  debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
  }

  if ((status = mailbox_open (mbox, MU_STREAM_READ)) != 0)
  {
    fprintf (stderr, "could not open  <%s>: %s\n",
      mailbox_name, strerror (status));
    exit (1);
  }

  mailbox_messages_count (mbox, &count);

  for (i = 1; i <= count; ++i)
  {
    message_t msg;
    header_t hdr;
    char subj[128];
    char date[128];
    size_t len = 0;

    if (
      (status = mailbox_get_message (mbox, i, &msg)) != 0 ||
      (status = message_get_header (msg, &hdr)) != 0)
    {
      fprintf (stderr, "msg %d : %s\n", i, strerror(status));
      exit(2);
    }

    if (
       (status = header_get_value (
          hdr, MU_HEADER_SUBJECT, subj, sizeof (subj), &len)) != 0 ||
      (status = header_get_value (
          hdr, MU_HEADER_DATE, date, sizeof (date), &len)) != 0)
    {
      fprintf (stderr, "msg %d : No Subject|Date\n", i);
      continue;
    }

    if (strcasecmp(subj, "WTLS 1.0 Daily Build-details") ==  0)
    {
      const char* s = date;
      struct tm   tm;
      char        dir[] = "yyyy.mm.dd";
      size_t      nparts = 0;
      size_t      partno;

      if((status = parse822_date_time(&s, s + strlen(s), &tm)))
      {
        fprintf (stderr, "parsing <%s> failed: %s\n", date, strerror(status));
        exit(1);
      }

      printf ("%d Processing for: year %d month %d day %d\n",
        i, tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      snprintf(dir, sizeof(dir), "%d.%02d.%02d",
        tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      status = mkdir(dir, 0777);

      if(status != 0)
      {
        switch(errno)
        {
        case EEXIST: /* we've already done this message */
          continue;
        case 0:
          break;
        default:
          fprintf (stderr, "mkdir %s failed: %s\n", dir, strerror(errno));
          status = 1;
          goto END;
          break;
        }
      }
      if((status = message_get_num_parts(msg, &nparts))) {
        fprintf (stderr, "get num parts failed: %s\n", strerror(status));
        break;
      }

      for(partno = 1; partno <= nparts; partno++)
      {
        message_t  part = NULL;
        char       content[128] = "<not found>";
        char*      fname = NULL;
        char       path[PATH_MAX];

        if((status = message_get_part(msg, partno, &part))) {
          fprintf (stderr, "get part failed: %s\n", strerror(status));
          break;
        }
        message_get_header (part, &hdr);
        header_get_value (hdr, MU_HEADER_CONTENT_DISPOSITION,
            content, sizeof (content), &len);

        fname = strrchr(content, '"');

        if(fname)
        {
          *fname = 0;

          fname = strchr(content, '"') + 1;

          snprintf(path, sizeof(path), "%s/%s", dir, fname);
          printf("  filename %s\n", path);

          if((status = message_save_attachment(part, path, NULL))) {
            fprintf (stderr, "save attachment failed: %s\n", strerror(status));
            break;
          }
        }
      }
      status = 0;
    }
  }

END:
  mailbox_close (mbox);
  mailbox_destroy (&mbox);

  return status;
}


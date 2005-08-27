/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/mailer.h>
#include <mailutils/message.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>

#define C(X) do {\
  int e;\
  if((e = X) != 0) { \
       fprintf(stderr, "%s failed: %s\n", #X, mu_strerror(e)); \
       exit(1);\
  }\
} while (0)

const char USAGE[] =
"usage: mailer [-hd] [-m mailer] [-f from] [to]..."
 ;
const char HELP[] =
"  -h    print this helpful message\n"
"  -m    a mailer URL (default is \"sendmail:\")\n"
"  -f    the envelope from address (default is from user environment)\n"
"  to    a list of envelope to addresses (default is from message)\n"
"\n"
"An RFC2822 formatted message is read from stdin and delivered using\n"
"the mailer.\n"
 ;

int
main (int argc, char *argv[])
{
  int opt;
  int optdebug = 0;
  char *optmailer = "sendmail:";
  char *optfrom = 0;

  mu_stream_t in = 0;
  mu_message_t msg = 0;
  mu_mailer_t mailer = 0;
  mu_address_t from = 0;
  mu_address_t to = 0;

  while ((opt = getopt (argc, argv, "hdm:f:")) != -1)
    {
      switch (opt)
        {
        case 'h':
          printf ("%s\n%s", USAGE, HELP);
          return 0;
          
        case 'd':
          optdebug++;
          break;
          
        case 'm':
          optmailer = optarg;
          break;
          
        case 'f':
          optfrom = optarg;
          break;

        default:
          fprintf (stderr, "%s\n", USAGE);
          exit (1);
        }
    }

  /* Register mailers. */
  mu_registrar_record (mu_smtp_record);
  mu_registrar_record (mu_sendmail_record);

  if (optfrom)
    {
      C (mu_address_create (&from, optfrom));
    }

  if (argv[optind])
    {
      char **av = argv + optind;

      C (mu_address_createv (&to, (const char **) av, -1));
    }

  C (mu_stdio_stream_create (&in, stdin, MU_STREAM_SEEKABLE));
  C (mu_stream_open (in));
  C (mu_message_create (&msg, NULL));
  C (mu_message_set_stream (msg, in, NULL));
  C (mu_mailer_create (&mailer, optmailer));

  if (optdebug)
    {
      mu_debug_t debug;
      mu_mailer_get_debug (mailer, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);
    }

  C (mu_mailer_open (mailer, 0));

  C (mu_mailer_send_message (mailer, msg, from, to));

  return 0;
}

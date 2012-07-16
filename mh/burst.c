/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005-2012 Free Software Foundation, Inc.

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

/* MH burst command */

#include <mh.h>

static char doc[] = N_("GNU MH burst")"\v"
N_("Options marked with `*' are not yet implemented.\n\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[MSGLIST]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",        ARG_FOLDER,        N_("FOLDER"), 0,
   N_("specify folder to operate upon")},
  {"inplace",      ARG_INPLACE,      N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("replace the source message with the table of contents, insert extracted messages after it") },
  {"noinplace",    ARG_NOINPLACE,    0, OPTION_HIDDEN, ""},
  {"quiet",        ARG_QUIET,        N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("be quiet about the messages that are not in digest format") },
  {"noquiet",      ARG_NOQUIET,      0, OPTION_HIDDEN, ""},
  {"verbose",      ARG_VERBOSE,      N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("verbosely list the actions taken") },
  {"noverbose",    ARG_NOVERBOSE,    0, OPTION_HIDDEN, ""},
  {"recursive",    ARG_RECURSIVE,    N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("recursively expand MIME messages") },
  {"norecursive",  ARG_NORECURSIVE,  0, OPTION_HIDDEN, ""},
  {"length",       ARG_LENGTH,       N_("NUMBER"), 0,
   N_("set minimal length of digest encapsulation boundary (default 1)") },
  { NULL }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "inplace",    MH_OPT_BOOL },
  { "quiet",      MH_OPT_BOOL },
  { "verbose",    MH_OPT_BOOL },
  { NULL }
};

/* Command line switches */
int inplace; 
int quiet;
int verbose;
int recursive;
int eb_min_length = 1;  /* Minimal length of encapsulation boundary */

#define VERBOSE(c) do { if (verbose) { printf c; putchar ('\n'); } } while (0)

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case ARG_INPLACE:
      inplace = is_true (arg);
      break;

    case ARG_NOINPLACE:
      inplace = 0;
      break;

    case ARG_LENGTH:
      eb_min_length = strtoul (arg, NULL, 0);
      if (eb_min_length == 0)
	eb_min_length = 1;
      break;
      
    case ARG_VERBOSE:
      verbose = is_true (arg);
      break;

    case ARG_NOVERBOSE:
      verbose = 0;
      break;

    case ARG_RECURSIVE:
      recursive = is_true (arg);
      break;
      
    case ARG_NORECURSIVE:
      recursive = 0;
      break;
      
    case ARG_QUIET:
      quiet = is_true (arg);
      break;

    case ARG_NOQUIET:
      quiet = 0;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* General-purpose data structures */
struct burst_map
{
  int mime;            /* Is mime? */
  size_t msgno;        /* Number of the original message */
  /* Following numbers refer to tmpbox */
  size_t first;        /* Number of the first bursted message */
  size_t count;        /* Number of bursted messages */
};


/* Global data */
struct burst_map map;        /* Currently built map */
struct burst_map *burst_map; /* Finished burst map */
size_t burst_count;          /* Number of items in burst_map */
mu_mailbox_t tmpbox;         /* Temporary mailbox */
mu_opool_t pool;             /* Object pool for building burst_map, etc. */

static int burst_or_copy (mu_message_t msg, int recursive, int copy);


/* MIME messages */
int 
burst_mime (mu_message_t msg)
{
  size_t i, nparts;
  
  mu_message_get_num_parts (msg, &nparts);

  for (i = 1; i <= nparts; i++)
    {
      mu_message_t mpart;
      if (mu_message_get_part (msg, i, &mpart) == 0)
	{
	  if (!map.first)
	    mu_mailbox_uidnext (tmpbox, &map.first);
	  burst_or_copy (mpart, recursive, 1);
	}
    }
  return 0;
}


/* Digest messages */

/* Bursting FSA states accoring to RFC 934:
   
      S1 ::   "-" S3
            | CRLF {CRLF} S1
            | c {c} S2

      S2 ::   CRLF {CRLF} S1
            | c {c} S2

      S3 ::   " " S2
            | c S4     ;; the bursting agent should consider the current
	               ;; message ended.  

      S4 ::   CRLF S5
            | c S4

      S5 ::   CRLF S5
            | c {c} S2 ;; The bursting agent should consider a new
	               ;; message started
*/

#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5

/* Negative state means no write */
int transtab[5][256] = {
/* S1 */ {  S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S1,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2, -S3,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2 },
/* S2 */ {  S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S1,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2 },
/* S3 */ { -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S2, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4 },
/* S4 */ { -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S5, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4, 
           -S4, -S4, -S4, -S4, -S4, -S4, -S4, -S4 },
/* S5 */ {  S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2, -S5,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2, 
            S2,  S2,  S2,  S2,  S2,  S2,  S2,  S2 }
};

#define F_FIRST  0x01  /* First part of the message (no EB seen so far) */
#define F_ENCAPS 0x02  /* Within encapsulated part */

struct burst_stream
{
  mu_stream_t stream;  /* Output stream */
  int flags;           /* See F_ flags above */
  size_t msgno;        /* Number of the current message */
  size_t partno;       /* Number of the part within the message */
};

static inline void
finish_stream (struct burst_stream *bs)
{
  if (bs->stream)
    {
      mu_message_t msg;
      
      mu_stream_seek (bs->stream, 0, SEEK_SET, NULL);
      msg = mh_stream_to_message (bs->stream);
      if (!map.first)
	mu_mailbox_uidnext (tmpbox, &map.first);
      burst_or_copy (msg, recursive, 1);
      mu_message_destroy (&msg, mu_message_get_owner (msg));
      bs->stream = 0;
      
      bs->partno++;
      bs->flags &= ~F_FIRST;
    }
}  

static inline void
flush_stream (struct burst_stream *bs, char *buf, size_t size)
{
  int rc;

  if (size == 0)
    return;
  if (!bs->stream)
    {
      if ((rc = mu_temp_file_stream_create (&bs->stream, NULL, 0))) 
	{
	  mu_error (_("Cannot open temporary file: %s"),
		    mu_strerror (rc));
	  exit (1);
	}
      mu_stream_printf (bs->stream, "X-Burst-Part: %lu %lu %02x\n",
			(unsigned long) bs->msgno,
			(unsigned long) bs->partno, bs->flags);
      if (!bs->flags)
	mu_stream_write (bs->stream, "\n", 1, NULL);

      if (verbose && !inplace)
	{
	  size_t nextuid;
	  mu_mailbox_uidnext (tmpbox, &nextuid);
	  printf (_("message %lu of digest %lu becomes message %lu\n"),
		  (unsigned long) bs->partno,
		  (unsigned long) bs->msgno,
		  (unsigned long) nextuid);
	}
    }
  rc = mu_stream_write (bs->stream, buf, size, NULL);
  if (rc)
    {
      mu_error (_("error writing temporary stream: %s"),
		mu_strerror (rc));
      exit (1); /* FIXME: better error handling please */
    }
}

/* Burst an RFC 934 digest.  Return 0 if OK, 1 if the message is not
   a valid digest.
   FIXME: On errors, cleanup and return -1.
*/
int
burst_digest (mu_message_t msg)
{
  mu_stream_t is;
  unsigned char c;
  size_t n;
  int state = S1;
  int eb_length = 0;
  struct burst_stream bs;
  int result = 0;
  
  bs.stream = NULL;
  bs.flags = F_FIRST;
  bs.partno = 1;
  mh_message_number (msg, &bs.msgno);
  
  mu_message_get_streamref (msg, &is);
  while (mu_stream_read (is, &c, 1, &n) == 0 && n == 1)
    {
      int newstate = transtab[state - 1][c];
      int silent = 0;
      
      if (newstate < 0)
	{
	  newstate = -newstate;
	  silent = 1;
	}

      if (state == S1)
	{
	  /* GNU extension: check if we have seen enough dashes to
	     constitute a valid encapsulation boundary. */
	  if (newstate == S3)
	    {
	      eb_length++;
	      if (eb_length < eb_min_length)
		continue; /* Ignore state change */
	      if (eb_min_length > 1)
		{
		  newstate = S4;
		  finish_stream (&bs);
		  bs.flags ^= F_ENCAPS;
		}
	    }
	  else
	    for (; eb_length; eb_length--)
	      flush_stream (&bs, "-", 1);
	  eb_length = 0;
	}
      else if (state == S5 && newstate == S2)
	{
	  /* As the automaton traverses from state S5 to S2, the
	     bursting agent should consider a new message started
	     and output the first character. */
	  finish_stream (&bs);
	}
      else if (state == S3 && newstate == S4)
	{
	  /* As the automaton traverses from state S3 to S4, the
	     bursting agent should consider the current message ended. */
	  finish_stream (&bs);
	  bs.flags ^= F_ENCAPS;
	}
      state = newstate;
      if (!silent)
	flush_stream (&bs, (char*)&c, 1);
    }
  mu_stream_destroy (&is);

  if (bs.flags == F_FIRST)
    {
      mu_stream_destroy (&bs.stream);
      result = 1;
    }
  else if (bs.stream)
    {
      mu_off_t size = 0;
      mu_stream_size (bs.stream, &size);
      if (size)
	finish_stream (&bs);
      else
	mu_stream_destroy (&bs.stream);
    }
  return result;
}


int
burst_or_copy (mu_message_t msg, int recursive, int copy)
{
  if (recursive)
    {
      int mime = 0;
      mu_message_is_multipart (msg, &mime);
  
      if (mime)
	{
	  if (!map.first)
	    map.mime = 1;
	  return burst_mime (msg);
	}
      else if (burst_digest (msg) == 0)
	return 0;
    }

  if (copy)
    {
      int rc;
      
      if (map.mime)
	{
	  mu_header_t hdr;
	  char *value = NULL;
	  
	  mu_message_get_header (msg, &hdr);
	  if (mu_header_aget_value (hdr, MU_HEADER_CONTENT_TYPE, &value) == 0
	      && memcmp (value, "message/rfc822", 14) == 0)
	    {
	      mu_stream_t str;
	      mu_body_t body;
	      
	      mu_message_get_body (msg, &body);
	      mu_body_get_streamref (body, &str);
	      msg = mh_stream_to_message (str);
	    }
	  free (value);
	}

      /* FIXME:
	 if (verbose && !inplace)
	   printf(_("message %lu of digest %lu becomes message %s"),
		   (unsigned long) (j+1),
		   (unsigned long) burst_map[i].msgno, to));
      */     

      rc = mu_mailbox_append_message (tmpbox, msg);
      if (rc)
	{
	  mu_error (_("cannot append message: %s"), mu_strerror (rc));
	  exit (1);
	}
      map.count++;
      return 0;
    }      
	      
  return 1;
}

int
burst (size_t num, mu_message_t msg, void *data)
{
  memset (&map, 0, sizeof (map));
  mh_message_number (msg, &map.msgno);
  
  if (burst_or_copy (msg, 1, 0) == 0)
    {
      VERBOSE((ngettext ("%s message exploded from digest %s",
			 "%s messages exploded from digest %s",
			 (unsigned long) map.count),
	       mu_umaxtostr (0, map.count),
	       mu_umaxtostr (1, num)));
      if (inplace)
	{
	  mu_opool_append (pool, &map, sizeof map);
	  burst_count++;
	}
    }
  else if (!quiet)
    mu_error (_("message %s not in digest format"), mu_umaxtostr (0, num));
  return 0;
}


/* Inplace handling */
struct rename_env
{
  size_t lastuid;
  size_t idx;
};

  
static int
_rename (size_t msgno, void *data)
{
  struct rename_env *rp = data;
  
  if (msgno == burst_map[rp->idx].msgno)
    {
      rp->lastuid -= burst_map[rp->idx].count;
      burst_map[rp->idx].msgno = rp->lastuid;
      rp->idx--;
    }

  if (msgno != rp->lastuid)
    {
      const char *from;
      const char *to;

      from = mu_umaxtostr (0, msgno);
      to   = mu_umaxtostr (1, rp->lastuid);
      --rp->lastuid;

      VERBOSE((_("message %s becomes message %s"), from, to));
	       
      if (rename (from, to))
	{
	  mu_error (_("error renaming %s to %s: %s"),
		    from, to, mu_strerror (errno));
	  exit (1);
	}
    }
  return 0;
}

void
burst_rename (mu_msgset_t ms, size_t lastuid)
{
  struct rename_env renv;

  VERBOSE ((_("Renaming messages")));
  renv.lastuid = lastuid;
  renv.idx = burst_count - 1;
  mu_msgset_foreach_dir_msguid (ms, 1, _rename, &renv);
}  

void
msg_copy (size_t num, const char *file)
{
  mu_message_t msg;
  mu_attribute_t attr = NULL;
  mu_stream_t istream, ostream;
  int rc;
  
  if ((rc = mu_file_stream_create (&ostream,
				   file,
				   MU_STREAM_WRITE|MU_STREAM_CREAT))) 
    {
      mu_error (_("Cannot open output file `%s': %s"),
		file, mu_strerror (rc));
      exit (1);
    }

  mu_mailbox_get_message (tmpbox, num, &msg);
  mu_message_get_streamref (msg, &istream);
  rc = mu_stream_copy (ostream, istream, 0, NULL);
  if (rc)
    {
      mu_error (_("copy stream error: %s"), mu_strerror (rc));
      exit (1);
    }
  
  mu_stream_destroy (&istream);

  mu_stream_close (ostream);
  mu_stream_destroy (&ostream);

  /* Mark message as deleted */
  mu_message_get_attribute (msg, &attr);
  mu_attribute_set_deleted (attr);
}

void
finalize_inplace (size_t lastuid)
{
  size_t i;

  VERBOSE ((_("Moving bursted out messages in place")));

  for (i = 0; i < burst_count; i++)
    {
      size_t j;

      /* FIXME: toc handling */
      for (j = 0; j < burst_map[i].count; j++)
	{
	  const char *to = mu_umaxtostr (0, burst_map[i].msgno + 1 + j);
	  VERBOSE((_("message %s of digest %s becomes message %s"),
		   mu_umaxtostr (1, j + 1),
		   mu_umaxtostr (2, burst_map[i].msgno), to));
	  msg_copy (burst_map[i].first + j, to);
	}
    }
}

int
main (int argc, char **argv)
{
  int index, rc;
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
  const char *tempfolder = mh_global_profile_get ("Temp-Folder", ".temp");
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  argc -= index;
  argv += index;

  VERBOSE ((_("Opening folder `%s'"), mh_current_folder ()));
  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);
  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");

  if (inplace)
    {
      size_t i, count;
      
      VERBOSE ((_("Opening temporary folder `%s'"), tempfolder));
      tmpbox = mh_open_folder (tempfolder, MU_STREAM_RDWR|MU_STREAM_CREAT);
      VERBOSE ((_("Cleaning up temporary folder")));
      mu_mailbox_messages_count (tmpbox, &count);
      for (i = 1; i <= count; i++)
	{
	  mu_attribute_t attr = NULL;
	  mu_message_t msg = NULL;
	  mu_mailbox_get_message (tmpbox, i, &msg);
	  mu_message_get_attribute (msg, &attr);
	  mu_attribute_set_deleted (attr);
	}
      mu_mailbox_expunge (tmpbox);
      mu_opool_create (&pool, 1);
    }
  else
    tmpbox = mbox;

  rc = mu_msgset_foreach_message (msgset, burst, NULL);
  if (rc)
    return rc;

  if (inplace && burst_count)
    {
      mu_url_t dst_url = NULL;
      size_t i, next_uid, last_uid;
      mu_msgset_t ms;
      size_t count;
      const char *dir;
      
      burst_map = mu_opool_finish (pool, NULL);

      mu_mailbox_uidnext (mbox, &next_uid);
      for (i = 0, last_uid = next_uid-1; i < burst_count; i++)
	last_uid += burst_map[i].count;
      VERBOSE ((_("Estimated last UID: %s"), mu_umaxtostr (0, last_uid)));

      rc = mu_msgset_create (&ms, mbox, MU_MSGSET_NUM);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_msgset_create", NULL, rc);
	  exit (1);
	}
      mu_mailbox_messages_count (mbox, &count);
      mu_msgset_add_range (ms, burst_map[0].msgno, count, MU_MSGSET_NUM);
	
      mu_mailbox_get_url (mbox, &dst_url);
      mu_url_sget_path (dst_url, &dir);
      VERBOSE ((_("changing to `%s'"), dir));
      if (chdir (dir))
	{
	  mu_error (_("cannot change to `%s': %s"), dir, mu_strerror (errno));
	  exit (1);
	}
      mu_mailbox_close (mbox);

      burst_rename (ms, last_uid);
      mu_msgset_free (ms);

      finalize_inplace (last_uid);

      VERBOSE ((_("Expunging temporary folder")));
      mu_mailbox_expunge (tmpbox);
      mu_mailbox_close (tmpbox);
      mu_mailbox_destroy (&tmpbox);
    }
  else
    mu_mailbox_close (mbox);

  mu_mailbox_destroy (&mbox);
  VERBOSE ((_("Finished bursting")));
  return rc;
}

    


/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Credits to the c-client and its Authors
 * The notorius c-client VALID() macro, was written by Mark Crispin.
 */

/* Parsing.
 * The approach is to detect the "From " as start of a
 * new message, give the position of the header and scan
 * until "\n" then set header_end, set body position, if we have
 * a Content-Length field jump to the point if not
 * scan until we it another "From " and set body_end.
 *
 ************************************
 * This is a classic case of premature optimisation
 * being the root of all Evil(Donald E. Knuth).
 * But I'm under "pressure" ;-) to come with
 * something "faster".  I think it's wastefull
 * to spend time to gain a few seconds on 30Megs mailboxes
 * ... but then again ... in computer time, 60 seconds, is eternity.
 *
 * If they use the event notification stuff
 * to get some headers/messages early ... it's like pissing
 * in the wind(sorry don't have the english equivalent).
 * The worst is progress_bar it should be ... &*($^ nuke.
 * For the events, we have to remove the *.LCK file,
 * release the locks, flush the stream save the pointers
 * etc ... hurry and wait...
 * I this point I'm pretty much ranting.
 *
 */

/* From the C-Client, part of pine */
/* You are not expected to understand this macro, but read the next page if
 * you are not faint of heart.
 *
 * Known formats to the VALID macro are:
 *              From user Wed Dec  2 05:53 1992
 * BSD          From user Wed Dec  2 05:53:22 1992
 * SysV         From user Wed Dec  2 05:53 PST 1992
 * rn           From user Wed Dec  2 05:53:22 PST 1992
 *              From user Wed Dec  2 05:53 -0700 1992
 *              From user Wed Dec  2 05:53:22 -0700 1992
 *              From user Wed Dec  2 05:53 1992 PST
 *              From user Wed Dec  2 05:53:22 1992 PST
 *              From user Wed Dec  2 05:53 1992 -0700
 * Solaris      From user Wed Dec  2 05:53:22 1992 -0700
 *
 * Plus all of the above with `` remote from xxx'' after it. Thank you very
 * much, smail and Solaris, for making my life considerably more complicated.
 */
/*
 * What?  You want to understand the VALID macro anyway?  Alright, since you
 * insist.  Actually, it isn't really all that difficult, provided that you
 * take it step by step.
 *
 * Line 1       Initializes the return ti value to failure (0);
 * Lines 2-3    Validates that the 1st-5th characters are ``From ''.
 * Lines 4-6    Validates that there is an end of line and points x at it.
 * Lines 7-14   First checks to see if the line is at least 41 characters long
.
 *              If so, it scans backwards to find the rightmost space.  From
 *              that point, it scans backwards to see if the string matches
 *              `` remote from''.  If so, it sets x to point to the space at
 *              the start of the string.
 * Line 15      Makes sure that there are at least 27 characters in the line.
 * Lines 16-21  Checks if the date/time ends with the year (there is a space
 *              five characters back).  If there is a colon three characters
 *              further back, there is no timezone field, so zn is set to 0
 *              and ti is set in front of the year.  Otherwise, there must
 *              either to be a space four characters back for a three-letter
 *              timezone, or a space six characters back followed by a + or -
 *              for a numeric timezone; in either case, zn and ti become the
 *              offset of the space immediately before it.
 * Lines 22-24  Are the failure case for line 14.  If there is a space four
 *              characters back, it is a three-letter timezone; there must be
a
 *              space for the year nine characters back.  zn is the zone
 *              offset; ti is the offset of the space.
 * Lines 25-28  Are the failure case for line 20.  If there is a space six
 *              characters back, it is a numeric timezone; there must be a
 *              space eleven characters back and a + or - five characters back
.
 *              zn is the zone offset; ti is the offset of the space.
 * Line 29-32   If ti is valid, make sure that the string before ti is of the
 *              form www mmm dd hh:mm or www mmm dd hh:mm:ss, otherwise
 *              invalidate ti.  There must be a colon three characters back
 *              and a space six or nine characters back (depending upon
 *              whether or not the character six characters back is a colon).
 *              There must be a space three characters further back (in front
 *              of the day), one seven characters back (in front of the month)
,
 *              and one eleven characters back (in front of the day of week).
 *              ti is set to be the offset of the space before the time.
 *
 * Why a macro?  It gets invoked a *lot* in a tight loop.  On some of the
 * newer pipelined machines it is faster being open-coded than it would be if
 * subroutines are called.
 *
 * Why does it scan backwards from the end of the line, instead of doing the
 * much easier forward scan?  There is no deterministic way to parse the
 * ``user'' field, because it may contain unquoted spaces!  Yes, I tested it t
o
 * see if unquoted spaces were possible.  They are, and I've encountered enoug
h
 * evil mail to be totally unwilling to trust that ``it will never happen''.
 */
#define VALID(s,x,ti,zn) {                      \
  ti = 0;                               \
  if ((*s == 'F') && (s[1] == 'r') && (s[2] == 'o') && (s[3] == 'm') && \
      (s[4] == ' ')) {                          \
    for (x = s + 5; *x && *x != '\n'; x++);             \
    if (x) {                                \
      if (x - s >= 41) {                        \
    for (zn = -1; x[zn] != ' '; zn--);              \
    if ((x[zn-1] == 'm') && (x[zn-2] == 'o') && (x[zn-3] == 'r') && \
        (x[zn-4] == 'f') && (x[zn-5] == ' ') && (x[zn-6] == 'e') && \
        (x[zn-7] == 't') && (x[zn-8] == 'o') && (x[zn-9] == 'm') && \
        (x[zn-10] == 'e') && (x[zn-11] == 'r') && (x[zn-12] == ' '))\
      x += zn - 12;                         \
      }                                 \
      if (x - s >= 27) {                        \
    if (x[-5] == ' ') {                     \
      if (x[-8] == ':') zn = 0,ti = -5;             \
      else if (x[-9] == ' ') ti = zn = -9;              \
      else if ((x[-11] == ' ') && ((x[-10]=='+') || (x[-10]=='-'))) \
        ti = zn = -11;                      \
    }                               \
    else if (x[-4] == ' ') {                    \
      if (x[-9] == ' ') zn = -4,ti = -9;                \
    }                               \
    else if (x[-6] == ' ') {                    \
      if ((x[-11] == ' ') && ((x[-5] == '+') || (x[-5] == '-')))    \
        zn = -6,ti = -11;                       \
    }                               \
    if (ti && !((x[ti - 3] == ':') &&               \
            (x[ti -= ((x[ti - 6] == ':') ? 9 : 6)] == ' ') &&   \
            (x[ti - 3] == ' ') && (x[ti - 7] == ' ') &&     \
            (x[ti - 11] == ' '))) ti = 0;           \
      }                                 \
    }                                   \
  }                                 \
}

#define STRLEN(s, i) \
do \
{ \
  char *tmp = (s);\
  while (*tmp) tmp++; \
  i = tmp - s; \
} while (0)

#define ATTRIBUTE_SET(buf,mum,c0,c1,type) \
do \
{ \
  char *s; \
  for (s = (buf) + 7; *s; s++) \
  { \
    if (*s == c0 || *s == c1) \
      { \
        (mum)->old_attr->flag |= (type); \
        break; \
      } \
  } \
} while (0)

#define ISSTATUS(buf) (\
(buf[0] == 'S' || buf[0] == 's') && \
(buf[1] == 'T' || buf[1] == 't') && \
(buf[2] == 'A' || buf[2] == 'a') && \
(buf[3] == 'T' || buf[3] == 't') && \
(buf[4] == 'U' || buf[4] == 'u') && \
(buf[5] == 'S' || buf[5] == 's') && (buf[6] == ':'))

/* notification */
#define MAILBOX_NOTIFICATION(mbox,which,bailing) \
do \
{ \
  size_t i; \
  event_t event; \
  for (i = 0; i < (mbox)->event_num; i++) \
    { \
      event = &((mbox)->event[i]); \
      if ((event->_action) &&  (event->type & (which))) \
        bailing |= event->_action (which, event->arg); \
    } \
} while (0)

/* notifications ADD_MESG */
#define DISPATCH_ADD_MSG(mbox,mud,file) \
do \
{ \
  int bailing = 0; \
  mailbox_unix_iunlock (mbox); \
  MAILBOX_NOTIFICATION (mbox, MU_EVT_MBX_MSG_ADD, bailing); \
  if (bailing != 0) \
    { \
      if (pcount) \
        *pcount = (mud)->messages_count; \
      fclose (file); \
      mailbox_unix_unlock (mbox); \
      return EINTR; \
    } \
  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK); \
} while (0);

/* notification MBX_PROGRESS
 * We do not want to fire up the progress notification
 * every line, it will be too expensive, so we do it
 * arbitrarely every 10 000 Lines.
 * FIXME: maybe this should be configurable.
 */
/*
 * This is more tricky we can not leave the mum
 * struct incomplete.  So we only tell them about
 * the complete messages.
 */
#define DISPATCH_PROGRESS(mbox,mud,file) \
do \
{ \
    { \
      int bailing = 0; \
      mailbox_unix_iunlock (mbox); \
      if (mud->messages_count != 0) \
        mud->messages_count--; \
      MAILBOX_NOTIFICATION (mbox, MU_EVT_MBX_PROGRESS,bailing); \
      if (bailing != 0) \
	{ \
	  if (pcount) \
	    *pcount = (mud)->messages_count; \
          fclose (file); \
          mailbox_unix_unlock (mbox); \
	  return EINTR; \
	} \
      if (mud->messages_count != 0) \
        mud->messages_count++; \
      mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK); \
    } \
} while (0)

/* skip a function call, ?? do we gain that much */
#define ATTRIBUTE_CREATE(attr,mbox) \
do \
{ \
  attr = calloc (1, sizeof(*(attr))); \
  if ((attr) == NULL) \
    { \
      fclose (file); \
      mailbox_unix_iunlock (mbox); \
      mailbox_unix_unlock (mbox); \
      return ENOMEM; \
    } \
} while (0)

//    size_t num = 2 * ((mud)->messages_count) + 10;
/* allocate slots for the new messages */
#define ALLOCATE_MSGS(mbox,mud,file) \
do \
{ \
  if ((mud)->messages_count >= (mud)->umessages_count) \
  { \
    mailbox_unix_message_t *m; \
    size_t num = ((mud)->umessages_count) + 1; \
    m = realloc ((mud)->umessages, num * sizeof (*m)); \
    if (m == NULL) \
      { \
        fclose (file); \
        mailbox_unix_iunlock (mbox); \
        mailbox_unix_unlock (mbox); \
        return ENOMEM; \
      } \
    (mud)->umessages = m; \
    (mud)->umessages[num - 1] = calloc (1, sizeof (*(mum))); \
    if ((mud)->umessages[num - 1] == NULL) \
      { \
        fclose (file); \
        mailbox_unix_iunlock (mbox); \
        mailbox_unix_unlock (mbox); \
        return ENOMEM; \
      } \
    ATTRIBUTE_CREATE (((mud)->umessages[num - 1])->old_attr, mbox); \
    (mud)->umessages_count = num; \
  } \
} while (0)

static int
mailbox_unix_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
#define MSGLINELEN 1024
  char buf[MSGLINELEN];
  int inheader;
  int inbody;
  off_t total;
  mailbox_unix_data_t mud;
  mailbox_unix_message_t mum = NULL;
  int status = 0;
  size_t lines;
  int newline;
  FILE *file;

  int zn, isfrom = 0;
  char *temp;

  /* sanity */
  if (mbox == NULL ||
      (mud = (mailbox_unix_data_t)mbox->data) == NULL)
    return EINVAL;

  /* the simplest way to deal with reentrancy issues is to
   * duplicate the FILE * pointer instead of the orignal.
   *
   * QnX4(and earlier ?) has a BUFSIZ of about 512 ???
   * we use setvbuf () to reset to something sane.
   * Really something smaller would be counter productive.
   */
  {
    file = fopen (mbox->name, "r");
    if (file == NULL)
      return errno;
#if BUFSIZ <= 1024
    {
      char *iobuffer;
      iobuffer = malloc (8192);
      if (iobuffer != NULL)
	if (setvbuf (file, iobuffer, _IOFBF, 8192) != 0)
	  free (iobuffer);
    }
#endif
  }

  /* grab the locks */
  mailbox_unix_ilock (mbox, MU_LOCKER_WRLOCK);
  mailbox_unix_lock (mbox, MU_LOCKER_RDLOCK);

  /* save the timestamp and size */
  {
    struct stat st;
    if (fstat (fileno (file), &st) != 0)
      {
	status = errno;
	fclose (file);
	mailbox_unix_iunlock (mbox);
	mailbox_unix_unlock (mbox);
	return status;
      }
    mud->mtime = st.st_mtime;
    mud->size = st.st_size;
  }

  /* seek to the starting point */
  if (mud->umessages)
    {
      if (mud->messages_count > 0 &&
	  msgno != 0 &&
	  msgno <= mud->messages_count)
	{
	  mum = mud->umessages[msgno - 1];
	  if (mum)
	    fseek (file, mum->body_end, SEEK_SET);
	  mud->messages_count = msgno;
	}
    }

  newline = 1;
  errno = total = lines = inheader = inbody = 0;

  while (fgets (buf, sizeof (buf), file))
    {
      int over, nl;
      STRLEN (buf, over);
      total += over;

      nl = (*buf == '\n') ? 1 : 0;
      VALID(buf, temp, isfrom, zn);
      isfrom = (isfrom) ? 1 : 0;

      /* which part of the message are we in ? */
      inheader = isfrom | ((!nl) & inheader);
      inbody = (!isfrom) & (!inheader);

      lines++;

      if (inheader)
	{
	  /* new message */
	  if (isfrom)
	    {
	      /* signal the end of the body */
	      if (mum && !mum->body_end)
		{
		  mum->body_end = total - over - newline;
		  mum->body_lines = --lines - newline;
		  DISPATCH_ADD_MSG(mbox, mud, file);
		}
	      /* allocate_msgs will initialize mum */
	      ALLOCATE_MSGS(mbox, mud, file);
	      mud->messages_count++;
	      mum = mud->umessages[mud->messages_count - 1];
	      mum->file = mud->file;
              mum->header_from = total - over;
              mum->header_from_end = total;
	      lines = 0;
	    }
	  else if ((over > 7) && ISSTATUS(buf))
	    {
	      mum->header_status = total - over;
	      mum->header_status_end = total;
	      ATTRIBUTE_SET(buf, mum, 'r', 'R', MU_ATTRIBUTE_READ);
	      ATTRIBUTE_SET(buf, mum, 'o', 'O', MU_ATTRIBUTE_SEEN);
	      ATTRIBUTE_SET(buf, mum, 'a', 'A', MU_ATTRIBUTE_ANSWERED);
	      ATTRIBUTE_SET(buf, mum, 'd', 'D', MU_ATTRIBUTE_DELETED);
	    }
	}

      /* body */
      if (inbody)
	{
	  /* set the body position */
	  if (mum && !mum->body)
	    {
	      mum->body = total - over + nl;
	      mum->header_lines = lines;
	      lines = 0;
	    }
	}

      newline = nl;

      /* every 50 mesgs update the lock, it should be every minute */
      if ((mud->messages_count % 50) == 0)
	mailbox_unix_touchlock (mbox);

      /* ping them every 1000 lines */
      if (((lines +1) % 1000) == 0)
	DISPATCH_PROGRESS(mbox, mud, file);

    } /* while */

  status = errno;

  /* not an error if we reach EOF */
  if (feof (file))
    status = 0;
  clearerr (file);

  if (mum)
    {
      mum->body_end = total - newline;
      mum->body_lines = lines - newline;
      DISPATCH_ADD_MSG(mbox, mud, file);
    }
  fclose (file);
  mailbox_unix_iunlock (mbox);
  mailbox_unix_unlock (mbox);
  if (pcount)
    *pcount = mud->messages_count;
  return status;
}


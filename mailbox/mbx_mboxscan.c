/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
   The approach is to detect the "From " as start of a new message, give the
   position of the header and scan until "\n" then set header_end, set body
   position, scan until we it another "From " and set body_end.
   ************************************
   This is a classic case of premature optimisation being the root of all
   Evil(Donald E. Knuth).  But I'm under "pressure" ;-) to come with
   something "faster".  I think it's wastefull * to spend time to gain a few
   seconds on 30Megs mailboxes ... but then again ... in computer time, 60
   seconds, is eternity.  If they use the event notification stuff to get
   some headers/messages early ... it's like pissing in the wind(sorry don't
   have the english equivalent).  The worst is progress_bar it should be ...
   &*($^ nuke.  For the events, we have to remove the *.LCK file, release the
   locks, flush the stream save the pointers  etc ... hurry and wait...
   I this point I'm pretty much ranting.  */


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

#define ATTRIBUTE_SET(buf,mum,c0,c1,type) \
do \
{ \
  char *s; \
  for (s = (buf) + 7; *s; s++) \
  { \
    if (*s == c0 || *s == c1) \
      { \
        (mum)->attr_flags |= (type); \
        break; \
      } \
  } \
} while (0)

#define ISCONTENT_TYPE(buf) (\
(buf[0] == 'C' || buf[0] == 'c') \
 && (buf[1] == 'O' || buf[1] == 'o') \
 && (buf[2] == 'N' || buf[2] == 'n') \
 && (buf[3] == 'T' || buf[3] == 't') \
 && (buf[4] == 'E' || buf[4] == 'e') \
 && (buf[5] == 'N' || buf[5] == 'n') \
 && (buf[6] == 'T' || buf[6] == 't') \
 && (buf[7] == '-') \
 && (buf[8] == 'T' || buf[8] == 't') \
 && (buf[9] == 'Y' || buf[9] == 'y') \
 && (buf[10] == 'P' || buf[10] == 'p') \
 && (buf[11] == 'E' || buf[11] == 'e') \
 && (buf[12] == ':' || buf[12] == ' ' || buf[12] == '\t'))

#define ISCC(buf) (\
(buf[0] == 'C' || buf[0] == 'c') \
 && (buf[1] == 'C' || buf[1] == 'c') \
 && (buf[2] == ':' || buf[2] == ' ' || buf[2] == '\t'))

#define ISDATE(buf) (\
(buf[0] == 'D' || buf[0] == 'd') \
 && (buf[1] == 'A' || buf[1] == 'a') \
 && (buf[2] == 'T' || buf[2] == 't') \
 && (buf[3] == 'E' || buf[3] == 'e') \
 && (buf[4] == ':' || buf[4] == ' ' || buf[4] == '\t'))

#define ISFROM(buf) (\
(buf[0] == 'F' || buf[0] == 'f') \
 && (buf[1] == 'R' || buf[1] == 'r') \
 && (buf[2] == 'O' || buf[2] == 'o') \
 && (buf[3] == 'M' || buf[3] == 'm') \
 && (buf[4] == ':' || buf[4] == ' ' || buf[4] == '\t'))

#define ISSTATUS(buf) (\
(buf[0] == 'S' || buf[0] == 's') \
 && (buf[1] == 'T' || buf[1] == 't') \
 && (buf[2] == 'A' || buf[2] == 'a') \
 && (buf[3] == 'T' || buf[3] == 't') \
 && (buf[4] == 'U' || buf[4] == 'u') \
 && (buf[5] == 'S' || buf[5] == 's') \
 && (buf[6] == ':' || buf[6] == ' ' || buf[6] == '\t'))

#define ISSUBJECT(buf) (\
(buf[0] == 'S' || buf[0] == 's') \
 && (buf[1] == 'U' || buf[1] == 'u') \
 && (buf[2] == 'B' || buf[2] == 'b') \
 && (buf[3] == 'J' || buf[3] == 'j') \
 && (buf[4] == 'E' || buf[4] == 'e') \
 && (buf[5] == 'C' || buf[5] == 'c') \
 && (buf[6] == 'T' || buf[6] == 't') \
 && (buf[7] == ':' || buf[7] == ' ' || buf[7] == '\t'))

#define ISTO(buf) (\
(buf[0] == 'T' || buf[0] == 't') \
 && (buf[1] == 'O' || buf[1] == 'o') \
 && (buf[2] == ':' || buf[2] == ' ' || buf[2] == '\t'))

#define ISX_IMAPBASE(buf) (\
(buf[0] == 'X' || buf[0] == 'x') \
 && (buf[1] == '-') \
 && (buf[2] == 'I' || buf[2] == 'i') \
 && (buf[3] == 'M' || buf[3] == 'm') \
 && (buf[4] == 'A' || buf[4] == 'a') \
 && (buf[5] == 'P' || buf[5] == 'p') \
 && (buf[6] == 'B' || buf[6] == 'b') \
 && (buf[7] == 'A' || buf[7] == 'a') \
 && (buf[8] == 'S' || buf[8] == 's') \
 && (buf[9] == 'E' || buf[9] == 'e') \
 && (buf[10] == ':' || buf[10] == ' ' || buf[10] == '\t'))

#define ISX_UIDL(buf) (\
(buf[0] == 'X' || buf[0] == 'x') \
 && (buf[1] == '-') \
 && (buf[2] == 'U' || buf[2] == 'u') \
 && (buf[3] == 'I' || buf[3] == 'i') \
 && (buf[4] == 'D' || buf[4] == 'd') \
 && (buf[5] == 'L' || buf[5] == 'l') \
 && (buf[6] == ':' || buf[6] == ' ' || buf[6] == '\t'))

#define ISX_UID(buf) (\
(buf[0] == 'X' || buf[0] == 'x') \
 && (buf[1] == '-') \
 && (buf[2] == 'U' || buf[2] == 'u') \
 && (buf[3] == 'I' || buf[3] == 'i') \
 && (buf[4] == 'D' || buf[4] == 'd') \
 && (buf[5] == ':' || buf[5] == ' ' || buf[5] == '\t'))

/* Skip prepen spaces.  */
#define SKIPSPACE(p) while (*p == ' ') p++

/* Save/concatenate the field-value in the field. */
#define FAST_HEADER(field,buf,n) \
do { \
  int i = 0; \
  char *s = field; \
  char *p = buf; \
  if (s) \
    while (*s++) i++; \
  else \
    p = memchr (buf, ':', n); \
  if (p) \
    { \
      int l; \
      char *tmp; \
      buf[n - 1] = '\0'; \
      p++; \
      if (!field) \
        SKIPSPACE(p); \
      l = n - (p - buf); \
      tmp = realloc (field, (l + i + 1) * sizeof (char)); \
      if (tmp) \
        { \
           field = tmp; \
           memcpy (field + i, p, l); \
        } \
    } \
} while (0)

#define FAST_HCONTENT_TYPE(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HCONTENT_TYPE],buf,n); \
sf = &(mum->fhdr[HCONTENT_TYPE])

#define FAST_HCC(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HCC],buf,n); \
sf = &(mum->fhdr[HCC])

#define FAST_HDATE(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HDATE],buf,n); \
sf = &(mum->fhdr[HDATE])

#define FAST_HFROM(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HFROM],buf,n); \
sf = &(mum->fhdr[HFROM])

#define FAST_HSUBJECT(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HSUBJECT],buf,n); \
sf = &(mum->fhdr[HSUBJECT])

#define FAST_HTO(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HTO],buf,n); \
sf = &(mum->fhdr[HTO])

#define FAST_HX_IMAPBASE(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HX_IMAPBASE],buf,n); \
sf = &(mum->fhdr[HX_UID])

#define FAST_HX_UIDL(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HX_UIDL],buf,n); \
sf = &(mum->fhdr[HX_UIDL])

#define FAST_HX_UID(mum,sf,buf,n) \
FAST_HEADER(mum->fhdr[HX_UID],buf,n); \
sf = &(mum->fhdr[HX_UID])

/* Notifications ADD_MESG. */
#define DISPATCH_ADD_MSG(mbox,mud) \
do \
{ \
  int bailing = 0; \
  monitor_unlock (mbox->monitor); \
  if (mbox->observable) \
     bailing = observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD); \
  if (bailing != 0) \
    { \
      if (pcount) \
        *pcount = (mud)->messages_count; \
      locker_unlock (mbox->locker); \
      return EINTR; \
    } \
  monitor_wrlock (mbox->monitor); \
} while (0);

/* Notification MBX_PROGRESS
   We do not want to fire up the progress notification every line, it will be
   too expensive, so we do it arbitrarely every 10 000 Lines.
   FIXME: maybe this should be configurable.  */
/* This is more tricky we can not leave the mum struct incomplete.  So we
   only tell them about the complete messages.  */
#define DISPATCH_PROGRESS(mbox,mud) \
do \
{ \
    { \
      int bailing = 0; \
      monitor_unlock (mbox->monitor); \
      mud->messages_count--; \
      if (mbox->observable) \
        bailing = observable_notify (mbox->observable, MU_EVT_MAILBOX_PROGRESS); \
      if (bailing != 0) \
	{ \
	  if (pcount) \
	    *pcount = (mud)->messages_count; \
          locker_unlock (mbox->locker); \
	  return EINTR; \
	} \
      mud->messages_count++; \
      monitor_wrlock (mbox->monitor); \
    } \
} while (0)

/* Allocate slots for the new messages.  */
/*    size_t num = 2 * ((mud)->messages_count) + 10; */
#define ALLOCATE_MSGS(mbox,mud) \
do \
{ \
  if ((mud)->messages_count >= (mud)->umessages_count) \
  { \
    mbox_message_t *m; \
    size_t num = ((mud)->umessages_count) + 1; \
    m = realloc ((mud)->umessages, num * sizeof (*m)); \
    if (m == NULL) \
      { \
        locker_unlock (mbox->locker); \
        monitor_unlock (mbox->monitor); \
        return ENOMEM; \
      } \
    (mud)->umessages = m; \
    (mud)->umessages[num - 1] = calloc (1, sizeof (*(mum))); \
    if ((mud)->umessages[num - 1] == NULL) \
      { \
        locker_unlock (mbox->locker); \
        monitor_unlock (mbox->monitor); \
        return ENOMEM; \
      } \
    (mud)->umessages_count = num; \
  } \
} while (0)

static int
mbox_scan0 (mailbox_t mailbox, size_t msgno, size_t *pcount, int do_notif)
{
#define MSGLINELEN 1024
  char buf[MSGLINELEN];
  int inheader;
  int inbody;
  off_t total = 0;
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum = NULL;
  int status = 0;
  size_t lines;
  int newline;
  size_t n = 0;
  stream_t stream;
  char **sfield = NULL;

  int zn, isfrom = 0;
  char *temp;

  /* Sanity.  */
  if (mud == NULL)
    return EINVAL;

  /* Grab the lock.  */
  monitor_wrlock (mailbox->monitor);

#ifdef WITH_PTHREAD
  /* read() is cancellation point since we're doing a potentially
     long operation.  Lets make sure we clean the state.  */
  pthread_cleanup_push (mbox_cleanup, (void *)mailbox);
#endif

  /* Save the timestamp and size.  */
  status = stream_size (mailbox->stream, &(mud->size));
  if (status != 0)
    {
      monitor_unlock (mailbox->monitor);
      return status;
    }

  locker_lock (mailbox->locker, MU_LOCKER_RDLOCK);

  /* Seek to the starting point.  */
  if (mud->umessages && msgno > 0 && mud->messages_count > 0
      && msgno <= mud->messages_count)
    {
      mum = mud->umessages[msgno - 1];
      if (mum)
	total = mum->header_from;
      mud->messages_count = msgno - 1;
    }
  else
    mud->messages_count = 0;

  newline = 1;
  errno = lines = inheader = inbody = 0;

  stream = mailbox->stream;
  while ((status = stream_readline (mailbox->stream, buf, sizeof (buf),
				    total, &n)) == 0 && n != 0)
    {
      int nl;
      total += n;

      nl = (*buf == '\n') ? 1 : 0;
      VALID(buf, temp, isfrom, zn);
      isfrom = (isfrom) ? 1 : 0;

      /* Which part of the message are we in ?  */
      inheader = isfrom | ((!nl) & inheader);
      inbody = (!isfrom) & (!inheader);

      if (buf[n - 1] == '\n')
	lines++;

      if (inheader)
	{
	  /* New message.  */
	  if (isfrom)
	    {
	      size_t j;
	      /* Signal the end of the body.  */
	      if (mum && !mum->body_end)
		{
		  mum->body_end = total - n - newline;
		  mum->body_lines = --lines - newline;
		  if (do_notif)
		    DISPATCH_ADD_MSG(mailbox, mud);
		}
	      /* Allocate_msgs will initialize mum.  */
	      ALLOCATE_MSGS(mailbox, mud);
	      mud->messages_count++;
	      mum = mud->umessages[mud->messages_count - 1];
	      mum->stream = mailbox->stream;
	      mum->mud = mud;
              mum->header_from = total - n;
              mum->header_from_end = total;
	      mum->body_end = mum->body = 0;
	      mum->attr_flags = 0;
	      lines = 0;
	      sfield = NULL;
	      for (j = 0; j < HDRSIZE; j++)
		if (mum->fhdr[j])
		  {
		    free (mum->fhdr[j]);
		    mum->fhdr[j] = NULL;
		  }
	    }
	  else if (ISSTATUS(buf))
	    {
	      ATTRIBUTE_SET(buf, mum, 'r', 'R', MU_ATTRIBUTE_READ);
	      ATTRIBUTE_SET(buf, mum, 'o', 'O', MU_ATTRIBUTE_SEEN);
	      ATTRIBUTE_SET(buf, mum, 'a', 'A', MU_ATTRIBUTE_ANSWERED);
	      ATTRIBUTE_SET(buf, mum, 'd', 'D', MU_ATTRIBUTE_DELETED);
	      sfield = NULL;
	    }
	  else if (ISCONTENT_TYPE(buf))
	    {
	      FAST_HCONTENT_TYPE(mum, sfield, buf, n);
	    }
	  else if (ISCC(buf))
	    {
	      FAST_HCC(mum, sfield, buf, n);
	    }
	  else if (ISDATE(buf))
	    {
	      FAST_HDATE(mum, sfield, buf, n);
	    }
	  else if (ISFROM(buf))
	    {
	      FAST_HFROM(mum, sfield, buf, n);
	    }
	  else if (ISSUBJECT(buf))
	    {
	      FAST_HSUBJECT (mum, sfield, buf, n);
	    }
	  else if (ISTO(buf))
	    {
	      FAST_HTO (mum, sfield, buf, n);
	    }
	  else if (ISX_IMAPBASE(buf))
	    {
	      FAST_HX_IMAPBASE (mum, sfield, buf, n);
	    }
	  else if (ISX_UIDL(buf))
	    {
	      FAST_HX_UIDL (mum, sfield, buf, n);
	    }
	  else if (ISX_UID(buf))
	    {
	      FAST_HX_UID (mum, sfield, buf, n);
	    }
	  else if (sfield && (buf[0] == ' ' || buf[0] == '\t'))
	    {
	      char *save = *sfield;
	      FAST_HEADER (save, buf, n);
	      *sfield = save;
	    }
	  else
	    {
	      sfield = NULL;
	    }
	}

      /* Body.  */
      if (inbody)
	{
	  /* Set the body position.  */
	  if (mum && !mum->body)
	    {
	      mum->body = total - n + nl;
	      mum->header_lines = lines;
	      lines = 0;
	    }
	}

      newline = nl;

      /* Every 100 mesgs update the lock, it should be every minute.  */
      if ((mud->messages_count % 100) == 0)
	locker_touchlock (mailbox->locker);

      /* Ping them every 1000 lines. Should be tunable.  */
      if (do_notif)
	if (((lines +1) % 1000) == 0)
	  DISPATCH_PROGRESS(mailbox, mud);

    } /* while */

  if (mum)
    {
      mum->body_end = total - newline;
      mum->body_lines = lines - newline;
      if (do_notif)
	DISPATCH_ADD_MSG(mailbox, mud);
    }
  if (pcount)
    *pcount = mud->messages_count;
  locker_unlock (mailbox->locker);
  monitor_unlock (mailbox->monitor);

  /* Reset the uidvalidity.  */
  if (mud->messages_count > 0)
    {
      mum = mud->umessages[0];
      if (mum->fhdr[HX_IMAPBASE])
	{
	  char *s = mum->fhdr[HX_IMAPBASE];
	  while (*s && !isdigit (*s)) s++;
	  mud->uidvalidity = strtoul (s, &s, 10);
	  mud->uidnext = strtoul (s, NULL, 10);
	}
      if (mud->uidvalidity == 0)
	{
	  char u[64];
	  mud->uidvalidity = (unsigned long)time (NULL);
	  mud->uidnext = mud->messages_count + 1;
	  if (mum->fhdr[HX_IMAPBASE])
	    free (mum->fhdr[HX_IMAPBASE]);
	  sprintf (u, "%lu %u", mud->uidvalidity, mud->uidnext);
	  mum->fhdr[HX_IMAPBASE] = strdup (u);
	  /* Tell that we have been modified for expunging.  */
	  mum->attr_flags |= MU_ATTRIBUTE_MODIFIED;
	}
    }
  /* Reset the IMAP uids, if necessary.  */
  {
    size_t uid;
    size_t ouid;
    size_t i;
    for (uid = ouid = i = 0; i < mud->messages_count; i++)
      {
	char *s;
	mum = mud->umessages[i];
	s = mum->fhdr[HX_UID];
	if (s)
	  {
	    while (*s && !isdigit (*s)) s++;
	    uid = strtoul (s, &s, 10);
	  }
	else
	  uid = 0;
	if (uid <= ouid)
	  {
	    char u[64];
	    uid = ouid + 1;
	    sprintf (u, "%d", uid);
	    if (mum->fhdr[HX_UID])
	      free (mum->fhdr[HX_UID]);
	    mum->fhdr[HX_UID] = strdup (u);
	    /* Note that we have modified for expunging.  */
	    mum->attr_flags |= MU_ATTRIBUTE_MODIFIED;
	  }
	mum->uid = ouid = uid;
      }
    if (uid >= mud->uidnext)
      {
	char u[64];
	mud->uidnext = uid + 1;
	mum = mud->umessages[0];
	if (mum->fhdr[HX_IMAPBASE])
	  free (mum->fhdr[HX_IMAPBASE]);
	sprintf (u, "%lu %u", mud->uidvalidity, uid + 1);
	mum->fhdr[HX_IMAPBASE] = strdup (u);
	mum->attr_flags |= MU_ATTRIBUTE_MODIFIED;
      }
  }
#ifdef WITH_PTHREAD
  pthread_cleanup_pop (0);
#endif
  return status;
}

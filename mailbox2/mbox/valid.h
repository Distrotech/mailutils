/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* Credits to the c-client and its Authors
 * The notorius c-client VALID() macro, was written by Mark Crispin.
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

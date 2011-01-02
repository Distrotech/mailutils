/* Generate FSA states for RFC 934 bursting agent.
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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

#include <stdio.h>

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

static int
token_num (int c)
{
  switch (c)
    {
    case '\n':
      return 1;
    case ' ':
      return 2;
    case '-':
      return 3;
    default:
      return 0;
    }
}

#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5

/* Negative state means no write */
int transtab[][4] = {
/*          DEF    '\n'   ' '   '-' */
/* S1 */ {  S2,    S1,    S2,   -S3 },
/* S2 */ {  S2,    S1,    S2,    S2 },
/* S3 */ { -S4,   -S4,   -S2,   -S4 }, 
/* S4 */ { -S4,   -S5,   -S4,   -S4 },
/* S5 */ {  S2,   -S5,    S2,    S2 }
};

int
main()
{
  int i, state;

  for (state = S1; state <= S5; state++)
    {
      printf ("/* S%d */ { ", state);
      for (i = 0; i < 256; i++)
	{
	  int newstate = transtab[state - 1][token_num (i)];

	  if (i > 0)
	    {
	      putchar (',');
	      if (i % 8 == 0)
		printf ("\n           ");
	    }
	  putchar (' ');
	  if (newstate < 0)
	    {
	      putchar ('-');
	      newstate = -newstate;
	    }
	  else
	    putchar (' ');
	  printf ("S%d", newstate);
	}
      printf (" },\n");
    }
  return 0;
}
  
    

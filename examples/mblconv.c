/* Conversion from "dot mailboxlist" format to Mailutils-3 subscription format.
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This file is free software; as a special exception the author gives
   unlimited permission to copy and/or distribute it, with or without
   modifications, as long as this notice is preserved.
   
   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   Use this program to convert IMAP4 subscriptions from Mailutils 2.2 and
   earlier to Mailutils-3 format.
   
   Usage:

     sort .mailboxlist | uniq | mblconv > .mu-subscr
     rm .mailboxlist
*/
#include <stdio.h>

int
main ()
{
  int c;
  
  while ((c = getchar ()) != EOF)
    {
      if (c == '\n')
	{
	  putchar (0);
	  putchar (0);
	}
      else
	putchar (c);
    }
  return 0;
}

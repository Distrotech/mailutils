CFLAGS = -Wall -pedantic -g
INCLUDES = -I../libmailbox
LIBS = ../libmailbox/.libs/libmailbox.al

from: from.c $(LIBS)
	gcc $(CFLAGS) $(INCLUDES) -o from from.c $(LIBS)

showmail: showmail.c $(LIBS)
	gcc $(CFLAGS) $(INCLUDES) -o showmail showmail.c $(LIBS)

clean:
	rm -f from showmail

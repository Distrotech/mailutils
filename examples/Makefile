CFLAGS = -Wall -pedantic -g
INCLUDES = -I../libmailbox
LIBS = ../libmailbox/.libs/libmailbox.al

from: from.c $(LIBS)
	gcc $(CFLAGS) $(INCLUDES) -o from from.c $(LIBS)

clean:
	rm -f from

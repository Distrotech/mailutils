CFLAGS = -Wall -pedantic -g
INCLUDES = -I../libmailbox
LIBS = ../libmailbox/.libs/libmailbox.al

showmail: showmail.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) -o showmail showmail.c $(LIBS)

addr: addr.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) -o addr addr.c $(LIBS)

clean:
	rm -f addr showmail

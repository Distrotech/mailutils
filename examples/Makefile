# Makefile

CFLAGS  = -g -I../include
LDFLAGS = -g -static
LIBS 	= ../mailbox/.libs/libmailbox.a ../lib/libmailutils.a

default: addr mbox-explode mbox-dates

# showmail

showmail: showmail.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# example of saving MIME parts to a file

mbox-explode: mbox-explode.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# example of parsing the date fields, prints all the incorrectly
# formatted dates in a mailbox.

mbox-dates: mbox-dates.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

bad-dates: mbox-dates
	for m in ~/Mail/*; do ./mbox-dates $$m; done | tee bad-dates.out

# addr example and test

test: addr
	./addr < Addrs > Addrs.test
	diff -u Addrs.good Addrs.test
	@echo "---- There should be no differences! ----"

addr: addr.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# clean and empty

clean:
	rm -f *.o

empty: clean
	rm -f addr showmail mbox-explode mbox-dates


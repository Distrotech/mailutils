# Makefile

CFLAGS  = -g -I../include -Wall
LDFLAGS = -g -static
LDLIBS 	= -lsocket

MULIBS	= ../mailbox/.libs/libmailbox.a ../lib/libmailutils.a

EXES    = addr mbox-explode mbox-dates mbox-auth url-parse mbox-check

default: $(EXES)

$(EXES): $(MULIBS)

# example of parsing the date fields, prints all the incorrectly
# formatted dates in a mailbox.

bad-dates: mbox-dates
	for m in ~/Mail/*; do ./mbox-dates $$m; done | tee bad-dates.out

# addr example and test

addr.test: addr
	./addr < Addrs > Addrs.test
	@echo "---- There should be no differences! ----"
	diff -u Addrs.good Addrs.test

# url example and test

url.test: url-parse
	./url-parse < Urls > Urls.test
	@echo "---- There should be no differences! ----"
	diff -u Urls.good Urls.test

# clean and empty

clean:
	rm -f *.o

empty: clean
	rm -f $(EXES)


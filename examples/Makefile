# Makefile

CFLAGS  = -g -I../include
LDFLAGS = -g -static
LIBS 	= ../mailbox/.libs/libmailbox.a ../lib/libmailutils.a

default: addr

# showmail

showmail: showmail.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

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
	rm -f addr showmail


# cmu-sieve/sv/Makefile

test: INBOX
	./Test 2>&1 | tee test.out

.PHONY: INBOX
INBOX:
	cp INBOX.orig INBOX

t-%: t-%.sv INBOX ../sieve
	../sieve -vvv -f INBOX $<

.PHONY: ../sieve
../sieve:
	make -C .. sieve


# cmu-sieve/sv/Makefile

default: test-complex

.PHONY: test-complex
test-complex:
	rm -f mbox.complex.*
	grep -v X-UID < mbox.complex > mbox.complex.in
	../sieve -k -f mbox.complex.in t-complex.sv

test: INBOX
	./Test 2>&1 | tee test.out

test-compile: ../sieve
	for s in ex-*.sv; do echo "Compiling: $$s"; ../sieve -c $$s; done

.PHONY: INBOX
INBOX:
	cp INBOX.orig INBOX

t-%: t-%.sv INBOX ../sieve
	../sieve -vvv -f INBOX $<

.PHONY: ../sieve
../sieve:
	make -C .. sieve


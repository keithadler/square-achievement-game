CC     ?= cc
CFLAGS ?= -O3 -march=native

all: sq sq2 sq3 sq4 sq3strict

sq: sq.c ; $(CC) $(CFLAGS) -o $@ $<
sq2: sq2.c ; $(CC) $(CFLAGS) -o $@ $<
sq3: sq3.c ; $(CC) $(CFLAGS) -o $@ $<
sq4: sq4.c ; $(CC) $(CFLAGS) -o $@ $<
sq3strict: sq3strict.c ; $(CC) $(CFLAGS) -o $@ $<

# Reproduce the headline result: every open case, n = 6..14.
result: sq3
	@for n in 6 7 8 9 10 11 12 13 14; do ./sq3 $$n 13 13 12; done

# Reproduce the published cases as a correctness check.
check: sq sq2
	./sq 3 10 12
	./sq 4 17 12
	./sq 5 26 12
	./sq2 4 17

clean: ; rm -rf sq sq2 sq3 sq4 sq3strict *.dSYM
.PHONY: all result check clean

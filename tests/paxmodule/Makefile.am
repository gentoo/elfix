ACLOCAL_AMFLAGS = -I m4

noinst_PROGRAMS = dummy
dummy_SOURCES = dummy.c

EXTRA_DIST = paxmodtest.sh

check_SCRIPTS = paxmodtest
TEST = $(check_SCRIPTS)

paxmodtest:
	./paxmodtest.sh 0 $(CFLAGS)

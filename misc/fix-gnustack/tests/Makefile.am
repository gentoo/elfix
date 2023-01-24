noinst_PROGRAMS = bad-gnustack

EXTRA_DIST = gnustacktest.sh

bad-gnustack.s: bad-gnustack.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $<
	$(SED) -i -e 's/GNU-stack,"",/GNU-stack,"x",/' $@

# Clang doesn't create executable stacks by default, so
# tests for fix-gnustack fail without this.
bad-gnustack$(EXEEXT): bad-gnustack.s
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-z,execstack -o $@ $<

check_SCRIPTS = gnustacktest
TEST = $(check_SCRIPTS)

gnustacktest: bad-gnustack
	$(top_srcdir)/tests/gnustacktest.sh

CLEANFILES = *.o *.s

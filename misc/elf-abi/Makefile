CFLAGS ?= -O2 -pipe -g
CFLAGS += -Wall
PWD = $$(pwd)

all: elf-abi

elf-abi: elf-abi.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

PREFIX = /usr
BINDIR = $(PREFIX)/bin
install: elf-abi
	install -m 755 -D elf-abi $(DESTDIR)$(BINDIR)/$<

clean:
	rm -f *.o *~ elf-abi

.PHONY: all clean install

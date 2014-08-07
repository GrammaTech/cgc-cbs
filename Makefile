PACKAGE_NAME 	= cgc-service-launcher
BINARY  	    = cb-server
MAN				= $(BINARY).1.gz
BIN		        = $(DESTDIR)/usr/bin
MANDIR			= $(DESTDIR)/usr/share/man/man1
OBJS    	    = main.o sockets.o signals.o privileges.o timeout.o utils.o resources.o
CC      	    = gcc
LD      	    = gcc

CFLAGS += -O3 -g -D_FORTIFY_SOURCE=2 -fstack-protector -fPIE
CFLAGS += -Werror -Wno-variadic-macros 
CFLAGS += -DRANDOM_UID -DHAVE_SETRESGID
# CFLAGS += -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error -Wno-disabled-macro-expansion

LDFLAGS += -Wl,-z,relro -Wl,-z,now

all: $(BINARY) man

$(BINARY): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

%.1.gz: %.md
	pandoc -s -t man $< -o $<.tmp
	gzip -9 < $<.tmp > $@

man: $(MAN)

install: $(BINARY) $(MAN)
	ls -la $(MAN)
	install -d $(BIN)
	install $(BINARY) $(BIN)
	install -d $(MANDIR)
	install $(MAN) $(MANDIR)

clean:
	-@rm -f *.o $(BINARY) $(MAN) *.tmp

distclean: clean

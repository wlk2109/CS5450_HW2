 # Makefile - Adapted from makefile for the gbn protocol

CC              = gcc
LD              = gcc
AR              = ar

CFLAGS          = -Wall -ansi -D_GNU_SOURCE 
LFLAGS          = -Wall -ansi

PROCESSOBJS		= p2pApp.o process_new.o
ALLEXEC			= process_new

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(ALLEXEC)

process: $(PROCESSOBJS)
	$(LD) $(LFLAGS) -o $@ $(PROCESSOBJS)

clean:
	rm -f *.o $(ALLEXEC)

realclean: clean
	rm -rf proj1.tar.gz

tarball: realclean
	tar cf - `ls -a | grep -v '^\.*$$' | grep -v '^proj[0-9].*\.tar\.gz'` | gzip > proj1-$(USER).tar.gz

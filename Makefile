# Adelaida
CC=gcc
CFLAGS=-c -g -Wall -std=c99
#LDFLAGS=-lreadline -Incurses #activar si se usa readline
 
SOURCES=nivelA.c nivelB.c nivelC.c nivelD.c my_shell.c
LIBRARIES= #.o
INCLUDES= #.h
PROGRAMS=nivelA nivelB nivelC nivelD my_shell
OBJS=$(SOURCES:.c=.o)
 
all: $(OBJS) $(PROGRAMS)
 
#$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
#	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@
 
nivelA: nivelA.o
	$(CC) $@.o -o $@ $(LIBRARIES)

nivelB: nivelB.o
	$(CC) $@.o -o $@ $(LIBRARIES)

nivelC: nivelC.o
	$(CC) $@.o -o $@ $(LIBRARIES)
	
nivelD: nivelD.o
	$(CC) $@.o -o $@ $(LIBRARIES)
 
my_shell: my_shell.o
	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)
 
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<
 
.PHONY: clean
clean:
	rm -rf *.o *~ *.tmp $(PROGRAMS)
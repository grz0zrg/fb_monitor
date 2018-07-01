CC=gcc
STANDARD_FLAGS=-std=c11 -pedantic -D_GNU_SOURCE -D_POSIX_SOURCE -Wno-unused-value -Wno-unknown-pragmas
DEBUG_FLAGS=-DDEBUG -g -Wall
RELEASE_FLAGS=-O2 -Wall
DEFP=-DFBG_PARALLEL
SRC_LIBS=../../src/lodepng/lodepng.c ../../src/fbgraphics.c
SRC=$(SRC_LIBS) octo_monitor.c
OUT=octo_monitor
LIBS=-lm -lpthread liblfds720.a -lcurl
INCS=-I ../../src/ -I.

all:
	$(CC) $(SRC) $(INCS) $(STANDARD_FLAGS) $(RELEASE_FLAGS) $(LIBS) -o $(OUT)

debug:
	$(CC) $(SRC) $(INCS) $(STANDARD_FLAGS) $(DEBUG_FLAGS) $(LIBS) -o $(OUT)

clean:
	rm -f *.o $(OUT)

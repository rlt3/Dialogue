UNAME:=$(shell uname)
MODULE:=dialogue
CFLAGS+=-Wall -std=c99 -pedantic -D _BSD_SOURCE -fPIC -Isrc/ -I./
LDFLAGS+=-L./ -L/usr/local/lib -lpthread -lreadline

SOURCES:=src/main.o src/console.o\
       src/dialogue.o \
       src/company.o src/tree.o \
       src/actor.o src/script.o \
       src/director.o src/worker.o

ifeq ($(UNAME), Linux)
	CFLAGS+=-I/usr/include/lua5.2/
	LDFLAGS+=-llua5.2 
endif

ifeq ($(UNAME), Darwin)
	CFLAGS+=-I/usr/local/include/
	LDFLAGS+=-llua
endif

all: clean build
check: clean build test

build: $(SOURCES)
	$(CC) $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

test:
	cd spec/ && ../$(MODULE) -s company.lua
	cd spec/ && ../$(MODULE) -s n_actor.lua
#   cd spec/ && ../$(MODULE) -s director.lua

mem:
	valgrind --leak-check=full --show-reachable=yes -v ./$(MODULE) -s spec/actor.lua

hel:
	valgrind --tool=helgrind -v ./$(MODULE) -s spec/actor.lua

tags:
	ctags -R -f tags .

clean:
	rm -f $(MODULE) src/*o

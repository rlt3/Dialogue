UNAME := $(shell uname)
CFLAGS+=-Wall -std=c99 -pedantic -D _BSD_SOURCE -fPIC -Isrc/ -I./
MODULE = dialogue

LDFLAGS+=-L./ -L/usr/local/lib -llua5.2 -lpthread -lreadline
SOURCES=src/main.o src/tree.o src/company.o src/actor.o src/script.o

ifeq ($(UNAME), Linux)
  CFLAGS+=-I/usr/include/lua5.2/
endif

ifeq ($(UNAME), Darwin)
  CFLAGS+=-I/usr/local/include/
endif

all: clean build
check: clean build test

build: $(SOURCES)
	$(CC) $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

test:
	cd spec/ && ../$(MODULE) company.lua
	cd spec/ && ../$(MODULE) actor.lua

mem:
	valgrind --leak-check=full -v ./$(MODULE) stage.lua

hel:
	valgrind --tool=helgrind -v ./$(MODULE) stage.lua

tags:
	ctags -R -f tags .

clean:
	rm -f $(MODULE) src/*o

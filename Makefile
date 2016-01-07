UNAME := $(shell uname)
CC=clang

SOURCES=src/dialogue.o src/director.o src/action.o src/worker.o src/utils.o

ifeq ($(DIALOGUE_HEADLESS), true)
  MODULE=Dialogue.so
else
  MODULE=dialogue
endif

ifeq ($(UNAME), Linux)
  ifeq ($(DIALOGUE_HEADLESS), true)
    SOFLAGS=-shared
  endif
  CFLAGS+=-Wall -Isrc/ -I./ -I/usr/include/lua5.2/ -D _BSD_SOURCE -fPIC
  LDFLAGS+=-L./ -L/usr/local/lib -llua5.2 -lpthread -lreadline
endif

ifeq ($(UNAME), Darwin)
  ifeq ($(DIALOGUE_HEADLESS), true)
    SOFLAGS=-bundle -undefined dynamic_lookup
  endif
  CFLAGS+=-Wall -Isrc/ -I./ -I/usr/local/include/ -D _BSD_SOURCE -fPIC
  LDFLAGS+=-L./ -L/usr/local/lib -llua -lpthread -lreadline
endif

all: clean build
check: clean build test

build: $(SOURCES)
	$(CC) -g $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

test:
	cp $(MODULE) spec/
	cd spec/ && busted test.lua

clean:
	rm -f $(MODULE) src/*o

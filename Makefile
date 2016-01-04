UNAME := $(shell uname)
CC=clang

ifeq ($(DIALOGUE_HEADLESS), true)
  MODULE=Dialogue.so
  SOURCES=src/action.o src/dialogue.o src/script.o src/actor.o src/luaf.o src/utils.o src/collection.o src/post.o src/postman.o src/mailbox.o
else
  MODULE=dialogue
  SOURCES=src/main.o src/action.o src/dialogue.o src/script.o src/actor.o src/luaf.o src/utils.o src/collection.o src/post.o src/postman.o src/mailbox.o
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
	cd spec/ && busted spec.lua

clean:
	rm -f $(MODULE) src/*o

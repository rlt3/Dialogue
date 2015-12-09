UNAME := $(shell uname)
CC=clang

ifeq ($(DIALOGUE_HEADLESS), true)
  MODULE=Dialogue.so
  SOURCES=src/dialogue.o src/actor.o src/actor_thread.o \
		  src/script.o src/mailbox.o src/utils.o \
		  src/tone.o
else
  MODULE=dialogue
  SOURCES=src/dialogue.o src/actor.o src/actor_thread.o \
		  src/script.o src/mailbox.o src/utils.o \
		  src/tone.o src/main.o src/interpreter.o
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

build: $(SOURCES)
	$(CC) -g $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

check:
	cp $(MODULE) spec/
	cd spec/ && busted spec.lua

test:
	valgrind --leak-check=full -v lua -i stage.lua

clean:
	rm -f $(MODULE) src/*o

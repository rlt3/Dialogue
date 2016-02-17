UNAME := $(shell uname)

SOURCES=src/main.o src/tree.o src/company.o src/actor.o src/script.o

ifeq ($(DIALOGUE_HEADLESS), true)
  MODULE=Dialogue.so
else
  MODULE=dialogue
  SOURCES+=src/main.o 
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

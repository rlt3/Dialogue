UNAME := $(shell uname)
CC=clang
SOURCES=src/dialogue.o src/actor.o src/actor_thread.o src/script.o src/utils.o
MODULE=Dialogue.so

ifeq ($(UNAME), Linux)
SOFLAGS=-shared
CFLAGS+=-Wall -Isrc/ -I./ -I/usr/include/lua5.2/ -D _BSD_SOURCE -fPIC
LDFLAGS+=-L./ -L/usr/local/lib -llua5.2 -lpthread
endif
ifeq ($(UNAME), Darwin)
SOFLAGS=-bundle -undefined dynamic_lookup
CFLAGS+=-Wall -Isrc/ -I./ -I/usr/local/include/ -D _BSD_SOURCE -fPIC
LDFLAGS+=-L./ -L/usr/local/lib -llua -lpthread
endif

all: clean build

build: $(SOURCES)
	$(CC) -g $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

check:
	cp $(MODULE) spec/
	cd spec/ && busted spec.lua

test:
	valgrind --leak-check=full -v lua -i stage.lua

actor: $(TMPSOURCES)
	$(CC) -g $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

clean:
	rm -f $(MODULE) *.so src/*o

UNAME := $(shell uname)
CC=clang
CFLAGS+=-Wall -Isrc/ -I./ -I/usr/local/include/ -D _BSD_SOURCE -fPIC
LDFLAGS+=-L./ -L/usr/local/lib
SOURCES=src/dialogue.o src/mailbox.o src/post.o src/actor.o src/script.o src/envelope.o src/utils.o
MODULE=Dialogue.so

ifeq ($(UNAME), Linux)
SOFLAGS=-shared
endif
ifeq ($(UNAME), Darwin)
SOFLAGS=-bundle -undefined dynamic_lookup
endif

all: clean dialogue

dialogue: $(SOURCES)
	$(CC) $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS) -llua -lpthread

test:
	busted spec.lua

check:
	valgrind --leak-check=full -v lua -i stage.lua

clean:
	rm -f $(EXECUTABLE) *.so src/*o

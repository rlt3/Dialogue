UNAME := $(shell uname)
CC=clang
SOURCES=src/dialogue.o src/mailbox.o src/postman.o src/tone.o src/post.o src/actor.o src/script.o src/envelope.o src/utils.o
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

all: clean dialogue

travis_build: $(SOURCES)
	$(CC) -Wall -Isrc/ -I./ -I./lua -I./lua/src -D _BSD_SOURCE -fPIC -shared -o $(MODULE) $^ -L./ -L/usr/lib -llua -lpthread

dialogue: $(SOURCES)
	$(CC) $(CFLAGS) $(SOFLAGS) -o $(MODULE) $^ $(LDFLAGS)

test:
	cp $(MODULE) spec/
	cd spec/ && busted spec.lua

check:
	valgrind --leak-check=full -v lua -i stage.lua

clean:
	rm -f $(EXECUTABLE) *.so src/*o

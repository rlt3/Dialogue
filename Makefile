CC=clang
CFLAGS+=-Wall -Isrc/ -I./ -I./lua -I./lua/src -D _BSD_SOURCE -fPIC
LDFLAGS+=-L./ -L/usr/lib
SOURCES=src/dialogue.o src/mailbox.o src/post.o src/actor.o src/script.o src/envelope.o src/utils.o
MODULE=Dialogue.so

all: clean dialogue

dialogue: $(SOURCES)
	$(CC) $(CFLAGS) -shared -o $(MODULE) $^ $(LDFLAGS) -llua -lpthread

test:
	busted spec.lua

check:
	valgrind --leak-check=full -v lua -i stage.lua

clean:
	rm -f $(EXECUTABLE) *.so src/*o

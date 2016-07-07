.PHONY: clean

CFLAGS  := -Wall -Werror -g
CC      := g++
LDFLAGS := ${LDFLAGS} -lpthread

APPS    := server binder client1

all: ${APPS}

server: rpc.cpp server_function_skels.c  server_functions.c server.c
	${CC} -o server rpc.cpp server_function_skels.c server_functions.c server.c  ${LDFLAGS}

client1: rpc.cpp client1.c
	${CC} -o client1 rpc.cpp client1.c ${LDFLAGS}

binder: rpc.cpp binder.cpp
	${CC} -o binder rpc.cpp binder.cpp ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

.PHONY: clean

CFLAGS  := -Wall -Werror -g
CC      := gcc
LDFLAGS := ${LDFLAGS} -lpthread

APPS    := server binder client

all: ${APPS}

server: rpc.cpp server_function_skels.c  server_functions.c server.c
	${CC} -o server rpc.cpp server_function_skels.c server_functions.c server.c  ${LDFLAGS}

client: rpc.cpp client1.c
	${CC} -o client1 rpc.cpp client1.c ${LDFLAGS}

binder: rpc.cpp binder.c
	${CC} -o binder rpc.cpp binder.c ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

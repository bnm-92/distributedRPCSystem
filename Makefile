.PHONY: clean

CC      := g++
CFLAGS  := -Wall -g
LDFLAGS := ${LDFLAGS} -lpthread

APPS    := server client1 binder

all: ${APPS}

server: server_function_skels.o  server_functions.o rpc.o server.c
	${CC} -o server rpc.cpp server_function_skels.c server_functions.c server.c  ${LDFLAGS}

client1: server_function_skels.o  server_functions.o rpc.o client1.c
	${CC} -o client1 rpc.cpp server_function_skels.c server_functions.c client1.c ${LDFLAGS}

binder: server_function_skels.o  server_functions.o rpc.o binder.cpp
	${CC} -o binder rpc.cpp server_function_skels.c server_functions.c binder.cpp ${LDFLAGS}

rpc.o:
	${CC} -c -o server_function_skels.o server_functions.o rpc.cpp  ${LDFLAGS}	

clean:
	rm -f *.o ${APPS}

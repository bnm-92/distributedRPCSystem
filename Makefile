.PHONY: clean

CC      := g++
CFLAGS  := -Wall -g
LDFLAGS := ${LDFLAGS} -lpthread

APPS    := binder rpc.o network.o librpc.a

all: ${APPS}

librpc.a: server_function_skels.o  server_functions.o network.o rpc.o
	ar rcs librpc.a rpc.o
	ar rcs librpc.a network.o

binder: server_function_skels.o  server_functions.o network.o rpc.o binder.cpp
	${CC} -o binder network.cpp rpc.cpp server_function_skels.c server_functions.c binder.cpp  -pthread ${LDFLAGS}

network.o:
	${CC} -c network.cpp -o network.o  -pthread ${LDFLAGS}	

rpc.o:
	${CC} -c rpc.cpp -o rpc.o server_function_skels.o server_functions.o -pthread ${LDFLAGS}	

clean:
	rm -f *.o ${APPS}

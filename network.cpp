#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

#define ARG_CHAR    1
#define ARG_SHORT   2
#define ARG_INT     3
#define ARG_LONG    4
#define ARG_DOUBLE  5
#define ARG_FLOAT   6

#define ARG_INPUT   31
#define ARG_OUTPUT  30

struct serverFunction {
    char* name;
    int* argTypes;
    int sockfd;
    char* address;
    int localfd;
    int numArgs;
};


int numBytes(int type, int arrLen) {
    int size = 0;
    if (type == 1) {
        size = sizeof(char);
    } else if (type == 2) {
        size = sizeof(short);
    } else if (type == 3) {
        size = sizeof(int);
    } else if (type == 4) {
        size = sizeof(long);
    } else if (type == 5) {
        size = sizeof(double);
    } else if (type == 6) {
        size = sizeof(float);
    }

    if (arrLen > 0) {
        size = size * arrLen;
    }

    return size;
}

void send_integer(int sockid, int val){
    int net_val = htonl(val);
    send(sockid, (char*)&net_val, 4, 0);
}

void send_string(int sockid, char* val){
    // For null terminator
    int len = strlen(val) + 1;
    int len_net = htonl(len);
    send(sockid, (char*)&len_net, 4, 0);

    send(sockid, val, len, 0);
}

int len_argTypes(int* argTypes){
    int i = 0;
    while(argTypes[i] != 0){
        i++;
    }
    return i + 1;
}

int is_input(int argType){
    if (argType & ( 1 << ARG_INPUT )) {
        return 1;
    }
    return 0;
}

int is_output(int argType){
    if (argType & ( 1 << ARG_OUTPUT )) {
        return 1;
    }
    return 0;
}

int get_arg_type(int argType){
    // second byte
    return (argType >> 16) & 0xff;
}

int get_arg_length(int argType){
    // lower two bytes
    return (((argType >> 8) & 0xff) << 8) | (argType & 0xff);
}

void send_argTypes(int sockid, int* argTypes){
    int len = len_argTypes(argTypes);
    int len_net = htonl(len);
    send(sockid, (char*)&len_net, 4, 0);

    for (int i=0; i<len; i++){
        int argType = argTypes[i];
        int argType_net = htonl(argType);
        send(sockid, (char*)&argType_net, 4, 0);
    }
}

void send_args(int sockid, int* argTypes, void** args){
    int len_at = len_argTypes(argTypes);
    for (int i=0; i<len_at-1; i++){
        int type = get_arg_type(argTypes[i]);
        int arg_len = get_arg_length(argTypes[i]);
        send(sockid, args[i], numBytes(type, arg_len), 0);
    }
}

void send_server(int sockid, serverFunction server){
    send_string(sockid, server.name);
    send_argTypes(sockid, server.argTypes);
    send_integer(sockid, server.sockfd);
    send_string(sockid, server.address);
    send_integer(sockid, server.localfd);
    send_integer(sockid, server.numArgs);
    //printf("func %s %d %s %d %d\n", server.name, server.sockfd, server.address, server.localfd, server.numArgs);
}

int recv_integer(int sockid){
    int val;
    int val_net;
    recv(sockid, &val_net, 4, 0);
    val = ntohl(val_net);
    return val;
}

char* recv_string(int sockid){
    int len = recv_integer(sockid);
    char * val = (char*)malloc(sizeof(char)*len);
    recv(sockid, val, len, 0);
    return val;
}

int* recv_argTypes(int sockid){
    int size_argTypes = recv_integer(sockid);
    int * argTypes = (int*)malloc(size_argTypes*sizeof(int));
    for (int i=0; i<size_argTypes; i++){
        int argType_net;
        recv(sockid, &argType_net, 4, 0);
        argTypes[i] = ntohl(argType_net);
    }     
    return argTypes;
}

void** recv_args(int sockid, int* argTypes){
    int num_args = len_argTypes(argTypes) - 1;
    void** args = (void **)malloc(num_args * sizeof(void *));
    for (int i=0; i<num_args; i++){
        int type = get_arg_type(argTypes[i]);
        int arg_len = get_arg_length(argTypes[i]);
        int* buf = (int*)malloc(numBytes(type, arg_len));
        recv(sockid, &(*buf), numBytes(type, arg_len), 0);
        args[i] = (void*)buf;
    }

    return args;
}

struct serverFunction* recv_servers(int sockid, int num_servers){
    struct serverFunction *servers = (serverFunction*)malloc(num_servers*sizeof(serverFunction));
    for (int i=0; i<num_servers; i++){
        struct serverFunction s;
        s.name = recv_string(sockid);
        s.argTypes = recv_argTypes(sockid);
        s.sockfd = recv_integer(sockid);
        s.address = recv_string(sockid);
        s.localfd = recv_integer(sockid);
        s.numArgs = recv_integer(sockid);
        servers[i] = s;
        //printf("func %s %d %s %d %d", s.name, s.sockfd, s.address, s.localfd, s.numArgs);
    }
    return servers;
}

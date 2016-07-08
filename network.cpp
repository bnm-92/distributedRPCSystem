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

#define ARG_INPUT   31
#define ARG_OUTPUT  30

void send_integer(int sockid, int val){
    int net_val = htonl(val);
    send(sockid, (char*)&net_val, 4, 0);
    printf("sent %d to %d\n", val, sockid);
}

void send_string(int sockid, char* val){
    int len = strlen(val);
    int len_net = htonl(len);
    send(sockid, (char*)&len_net, 4, 0);
    printf("sent %d to %d\n", len, sockid);

    send(sockid, val, len, 0);
    printf("sent %s to %d\n", val, sockid);
}

int len_argTypes(int* argTypes){
    int i = 0;
    while(argTypes[i] != 0){
        i++;
    }
    return i + 1;
}

int get_arg_input_type(int argType){
    // first bit 1 -> 128, second bit 1 -> 64
    if (((argType >> 24) & 0xff) == 128){
        return ARG_INPUT;
    }
    return ARG_OUTPUT;
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
    printf("sent %d to %d\n", len, sockid);

    for (int i=0; i<len; i++){
        int argType = argTypes[i];
        int argType_net = htonl(argType);
        printf("argTypes %d\n", argType);
        send(sockid, (char*)&argType_net, 4, 0);
    }
}

void send_args(int sockid, int* argTypes, void** args){
    printf("ArgType %d\n", argTypes[0]);
    printf("%d\n", get_arg_input_type(argTypes[0]));
    printf("%d\n", get_arg_type(argTypes[0]));
    printf("%d\n", get_arg_length(argTypes[0]));
}

int recv_integer(int sockid){
    int val;
    int val_net;
    recv(sockid, &val_net, 4, 0);
    val = ntohl(val_net);
    printf("received %d from %d\n", val, sockid);
    return val;
}

char* recv_string(int sockid){
    int len = recv_integer(sockid);
    char * val = (char*)malloc(sizeof(char)*len);
    recv(sockid, val, len, 0);
    printf("received %s from %d\n", val, sockid);
    return val;
}

int* recv_argTypes(int sockid){
    int size_argTypes = recv_integer(sockid);
    printf("argType len %d\n", size_argTypes);
    int * argTypes = (int*)malloc(size_argTypes*sizeof(int));
    for (int i=0; i<size_argTypes; i++){
        int argType_net;
        recv(sockid, &argType_net, 4, 0);
        argTypes[i] = ntohl(argType_net);
        printf("argType %d\n", argTypes[i]);
    }     
    return argTypes;
}

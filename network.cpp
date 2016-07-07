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

void send_argTypes(int sockid, int* argTypes){
    int len_argTypes = sizeof(argTypes);
    int len_argTypes_net = htonl(len_argTypes);
    send(sockid, (char*)&len_argTypes_net, 4, 0);
    printf("sent %d to %d\n", len_argTypes, sockid);

    for (int i=0; i<len_argTypes/2; i++){
        int argType = argTypes[i];
        int argType_net = htonl(argType);
        printf("argTypes %d\n", argType);
        send(sockid, (char*)&argType_net, 2, 0);
    }
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
    int len_argTypes = recv_integer(sockid);
    int argTypes[len_argTypes/2];
    for (int i=0; i<len_argTypes/2; i++){
        int argType_net;
        recv(sockid, &argType_net, 2, 0);
        argTypes[i] = ntohl(argType_net);
        printf("argType %d\n", argTypes[i]);
    }     

}
#include "rpc.h"

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
#include <pthread.h>

void *get_in_addr(struct sockaddr *sa);

int rpcInit(){
    // this will create a socket for a server with a random 
    //port and connect to the binder
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    char* hostname = getenv("BINDER_ADDRESS");
    char* port  = getenv("BINDER_PORT");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, (char*)port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("server: connecting to %s\n", s);

    freeaddrinfo(servinfo);
    return sockfd;
}

int connectToSocket(int port, hostent* server){
    int sockfd;
    struct sockaddr_in server_address;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
        (char *)&server_address.sin_addr.s_addr,
        server->h_length);
    server_address.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *) &server_address,sizeof(server_address)) < 0){
        return -1;
    }
    return sockfd;
}

char* toString(void* data, int type){
    int length_in_bytes[] = {0,1,2,4,4,8,4};
    char t = type >> 16;
    char* ret = (char*)malloc(sizeof(char)*length_in_bytes[t]);
    switch(t){
        case ARG_CHAR: {
            char c = *(char*) data;
            ret[0] = c;
            return ret;
        }
        case ARG_SHORT: {
            short s = *(short*) data;
            ret[0] = (char) ((s >> 8) & 0xff);
            ret[1] = (char) (s & 0xff);
            return ret;
        }
        case ARG_INT: {
            int i = *(int*) data;
            printf("i %d\n", i);
            ret[0] = (char) ((i >> 24) & 0xff);
            ret[1] = (char) ((i >> 16) & 0xff);
            ret[2] = (char) ((i >> 8) & 0xff);
            ret[3] = (char) (i & 0xff);
            return ret;
        }
        case ARG_LONG: {
            long l = *(long*) data;
            ret[0] = (char) ((l >> 24) & 0xff);
            ret[1] = (char) ((l >> 16) & 0xff);
            ret[2] = (char) ((l >> 8) & 0xff);
            ret[3] = (char) (l & 0xff);
            return ret;
        }
        // Need to find the best way to do this
        // Shifting doesn't work for doubles
        case ARG_DOUBLE: {
            double d = *(double*) data;
            memcpy(&ret,&d,sizeof(d));
            return ret;
        }
        default: {
            long f = *(float*) data;
            memcpy(&ret,&f,sizeof(f));
            return ret;
       }
    }
}

int rpcCall(char* name, int* argTypes, void** args){
    // Connect to binder
    int binder_port = atoi(getenv("BINDER_PORT"));
    struct hostent *binder_address = gethostbyname(getenv("BINDER_ADDRESS"));

    int sockfd = connectToSocket(binder_port, binder_address); 
    if (sockfd < 0){
        return -1;
    }

    // request_msg format: LOC_REQUEST name argTypes  
    int i;
    char request_msg[sizeof(LOC_REQUEST) + sizeof(name) + sizeof(argTypes) + 2];
    char str[4];
    sprintf(str, "%d", LOC_REQUEST);
    strcpy(request_msg, str);
    strcat(request_msg, " ");
    strcat(request_msg, name);
    strcat(request_msg, " ");
    for (i=0; i <= sizeof(argTypes)/sizeof(argTypes[0]); i++){
        char ret[4];
        ret[0] = (char) (argTypes[i] >> 24) & 0xff;
        ret[1] = (char) (argTypes[i] >> 16) & 0xff;
        ret[2] = (char) (argTypes[i] >>  8) & 0xff;
        ret[3] = (char) argTypes[i] & 0xff;
        strcat(request_msg, ret);
    }
    printf("%s", request_msg);
    int n = write(sockfd, request_msg, strlen(request_msg)); 
   
    char buffer[256];
    n = read(sockfd, buffer, 255);
    if (n < 0){
        return -1;
    }
    if (buffer[0] != LOC_SUCCESS){
        return -1;
    }
    close(sockfd);
    
    struct hostent *server_identifier = gethostbyname(strtok(buffer, " ")); 
    int server_port = atoi(strtok(buffer, " "));

    sockfd = connectToSocket(server_port, server_identifier);
    if (sockfd < 0){
        return -1;
    }

    // msg format: EXECUTE name argTypes args
    char response_msg[sizeof(EXECUTE) + sizeof(name) + sizeof(argTypes) + sizeof(args) + 3];
    sprintf(str, "%d", EXECUTE);
    strcpy(request_msg, str);
    strcat(response_msg, " ");
    strcat(response_msg, name);
    strcat(response_msg, " ");
    for (i=0; i <= sizeof(argTypes)/sizeof(argTypes[0]); i++){
        char ret[4];
        ret[0] = (char) (argTypes[i] >> 24) & 0xff;
        ret[1] = (char) (argTypes[i] >> 16) & 0xff;
        ret[2] = (char) (argTypes[i] >>  8) & 0xff;
        ret[3] = (char) argTypes[i] & 0xff;
        strcat(request_msg, ret);
    }
    strcat(response_msg, " ");
    for (i=0; i <= sizeof(argTypes)/sizeof(argTypes[0]); i++){
        strcat(response_msg, toString(args[i],argTypes[i]));
    }
    n = write(sockfd, response_msg, strlen(response_msg));

    n = read(sockfd, buffer, 255);
    if (n < 0){
        return -1;
    }
    if (buffer[0] != EXECUTE_SUCCESS){
        return -1;
    }
    close(sockfd);
    return 0;
}

int rpcCacheCall(char* name, int* argTypes, void** args){
    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f){
    return 0;
}

int rpcExecute(){
    return 0;
}

int rpcTerminate(){
    return 0;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


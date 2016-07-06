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

#define PORT_ZERO 0

int sockfdBinder;
fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;        // maximum file descriptor number
int listener;     // listening socket descriptor
int newfd;        // newly accept()ed socket descriptor
struct sockaddr_storage remoteaddr; // client address
socklen_t addrlen;
pthread_t clientThread;

char* SERVER_ADDRESS;
int PORT;

void *listenForClient(void * id) {
    int i;
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    fdmax = listener;

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                    }
                } else {
                    // printf("handle data\n");
                    
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
} // end of function

int rpcInit(){
    // this will create a socket for the client and listen to it on a thread

    char* buf;
    int nbytes;
    // char remoteIP[INET6_ADDRSTRLEN];
    char hostIP[INET6_ADDRSTRLEN];
    int i, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT_ZERO, &hints, &ai)) != 0) {
        // fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    int port_num;
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        // setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

            int ret = bind(listener, p->ai_addr, p->ai_addrlen);
            getsockname(listener, (struct sockaddr *)p->ai_addr, (socklen_t *)&p->ai_addrlen);
            port_num = getPort((struct sockaddr *)p->ai_addr);
        if (ret < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    // print your server name and port here
    gethostname(hostIP, INET6_ADDRSTRLEN);
    
    SERVER_ADDRESS = hostIP;
    PORT = htons(port_num);

    freeaddrinfo(ai); // all done with this

    //create a thread for listening purposes
    pthread_create(&clientThread, NULL, listenForClient, (void*)0);

    // connect to the binder with a separate socket and keep it open
    struct addrinfo hintsB, *servinfoB, *pB;
    int rvB;
    char sB[INET6_ADDRSTRLEN];

    char* hostname = getenv("BINDER_ADDRESS");
    char* port  = getenv("BINDER_PORT");

    memset(&hintsB, 0, sizeof hintsB);
    hintsB.ai_family = AF_UNSPEC;
    hintsB.ai_socktype = SOCK_STREAM;

    if ((rvB = getaddrinfo(hostname, (char*)port, &hintsB, &servinfoB)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(pB = servinfoB; pB != NULL; pB = pB->ai_next) {
        if ((sockfdBinder = socket(pB->ai_family, pB->ai_socktype,
                pB->ai_protocol)) == -1) {
            // perror("server: socket");
            continue;
        }

        if (connect(sockfdBinder, pB->ai_addr, pB->ai_addrlen) == -1) {
            close(sockfdBinder);
            // perror("server: connect");
            continue;
        }

        break;
    }

    if (pB == NULL) {
        fprintf(stderr, "server: failed to connect\n");
        return 2;
    }

    inet_ntop(pB->ai_family, get_in_addr((struct sockaddr *)pB->ai_addr), sB, sizeof sB);
    printf("server: connecting to %s\n", sB);
    // while(1) {
    //     //do some work
    // }
    freeaddrinfo(servinfoB);
    return 0;
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

int rpcCall(char* name, int* argTypes, void** args){
    printf("START\n");
    // Connect to binder
    int binder_port = atoi(getenv("BINDER_PORT"));
    struct hostent *binder_address = gethostbyname(getenv("BINDER_ADDRESS"));
    int sockfd = connectToSocket(binder_port, binder_address); 
    if (sockfd < 0){
        return -1;
    }

    // request_msg format: LOC_REQUEST len_name name len_argTypes argTypes  
    int i;

    printf("loc_request %d\n", LOC_REQUEST);
    int loc_request_net = htonl(LOC_REQUEST);
    send(sockfd, (char*)&loc_request_net, 4, 0);

    int len_name = strlen(name);
    int len_name_net = htonl(len_name);
    printf("size of name %d\n", len_name);
    send(sockfd, (char*)&len_name_net, sizeof(len_name_net), 0);

    printf("name %s\n", name);
    send(sockfd, name, len_name, 0);

    int len_argTypes = sizeof(argTypes);
    int len_argTypes_net = htonl(len_argTypes);
    printf("size of argTypes %d\n", len_argTypes);
    send(sockfd, (char*)&len_argTypes_net, 4, 0);

    for (i=0; i<len_argTypes/2; i++){
        int argType = argTypes[i];
        int argType_net = htonl(argType);
        printf("argTypes %d\n", argType);
        send(sockfd, (char*)&argType_net, 2, 0);
    }

    printf("done\n");

    int code;
    int code_net;

    // Get code
    if (recv(sockfd, &code_net, 4, 0) <= 0){
        return -1;
    }
    code = ntohl(code_net);
    printf("code %d\n", code);
    if (code != LOC_SUCCESS){
        return -1;
    }
    // Get server port
    int server_port_net;
    int server_port;
    recv(sockfd, &server_port_net, 4, 0);
    server_port = ntohl(server_port_net);
    printf("server_port %d\n", server_port);

    //Get server address length
    int len_server_addr_net;
    int len_server_addr;
    recv(sockfd, &len_server_addr_net, 4, 0);
    len_server_addr = ntohl(len_server_addr_net);
    printf("len_server_addr %d\n", len_server_addr);

    // Get server address
    char * server_addr = (char*)malloc(sizeof(char)*len_server_addr);
    recv(sockfd, server_addr, len_server_addr, 0);
    printf("server_addr %s\n", server_addr);


    
    close(sockfd);
    
    // struct hostent *server_identifier = gethostbyname(strtok(buffer, " ")); 
    // int server_port = atoi(strtok(buffer, " "));

    // sockfd = connectToSocket(server_port, server_identifier);
    // if (sockfd < 0){
    //     return -1;
    // }

    // // msg format: Length EXECUTE name argTypes args
    // char response_msg[sizeof(EXECUTE) + sizeof(name) + sizeof(argTypes) + sizeof(args) + 3];
    // sprintf(str, "%d", EXECUTE);
    // strcpy(request_msg, str);
    // strcat(response_msg, " ");
    // strcat(response_msg, name);
    // strcat(response_msg, " ");
    // for (i=0; i <= sizeof(argTypes)/sizeof(argTypes[0]); i++){
    //     char ret[4];
    //     ret[0] = (char) (argTypes[i] >> 24) & 0xff;
    //     ret[1] = (char) (argTypes[i] >> 16) & 0xff;
    //     ret[2] = (char) (argTypes[i] >>  8) & 0xff;
    //     ret[3] = (char) argTypes[i] & 0xff;
    //     strcat(request_msg, ret);
    // }
    // strcat(response_msg, " ");
    // for (i=0; i <= sizeof(argTypes)/sizeof(argTypes[0]); i++){
    //     strcat(response_msg, toString(args[i],argTypes[i]));
    // }
    // n = write(sockfd, response_msg, strlen(response_msg));

    // n = read(sockfd, buffer, 255);
    // if (n < 0){
    //     return -1;
    // }
    // if (buffer[0] != EXECUTE_SUCCESS){
    //     return -1;
    // }
    // close(sockfd);

    free(server_addr);
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

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int getPort(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);   
}

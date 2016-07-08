#include "rpc.h"
#include "network.h"
#include "server_function_skels.h"
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
#include <vector>

using namespace std;

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

// Define some types
int char_output = (1 << ARG_OUTPUT) | (ARG_CHAR << 16);
int char_input = (1 << ARG_INPUT) | (ARG_CHAR << 16);
int short_output = (1 << ARG_OUTPUT) | (ARG_SHORT << 16);
int short_input = (1 << ARG_INPUT) | (ARG_SHORT << 16);
int int_output = (1 << ARG_OUTPUT) | (ARG_INT << 16);
int int_input = (1 << ARG_INPUT) | (ARG_INT << 16);
int long_output = (1 << ARG_OUTPUT) | (ARG_LONG << 16);
int long_input = (1 << ARG_INPUT) | (ARG_LONG << 16);
int double_output = (1 << ARG_OUTPUT) | (ARG_DOUBLE << 16);
int double_input = (1 << ARG_INPUT) | (ARG_DOUBLE << 16);
int float_output = (1 << ARG_OUTPUT) | (ARG_FLOAT << 16);
int float_input = (1 << ARG_INPUT) | (ARG_FLOAT << 16);

struct serverFunction {
	char* name;
	int* argTypes;
	int sockfd;
    char* address;
    int localfd;
};

vector<serverFunction> cache;

int getargTypesLength(int* argTypes) {
	// write code here
	int n = 0;
	while (*argTypes != 0) {
		argTypes++;
		n++;
	}

	return n;
}

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
                    int code;
                    int code_net;

                    // Get code
                    int nbytes;
                    if ((nbytes = recv(i, &code_net, 4, 0)) <= 0) {
                        //got error or connection closed by client
                        if (nbytes == 0) {
                            //connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        break;
                    }
                    code = ntohl(code_net);
                    printf("code %d\n", code);

                    if (code == EXECUTE){
                        // length of name
                        int len_name_net;
                        int len_name;
                        recv(i, &len_name_net, 4, 0);
                        len_name = ntohl(len_name_net);
                        printf("len_name %d\n", len_name);

                        // name
                        char * name = (char*)malloc(sizeof(char)*len_name);
                        recv(i, name, len_name, 0);
                        printf("name %s\n", name);

                        // length of argTypes
                        int len_argTypes_net;
                        int len_argTypes;
                        recv(i, &len_argTypes_net, 4, 0);
                        len_argTypes = ntohl(len_argTypes_net);
                        printf("len_argTypes %d\n", len_argTypes);

                        int argTypes[len_argTypes/2];
                        int j;
                        for (j=0; j<len_argTypes/2; j++){
                            int argType_net;
                            recv(i, &argType_net, 4, 0);
                            argTypes[j] = ntohl(argType_net);
                            printf("argType %d\n", argTypes[j]);
                            printf("argType net %d\n", argType_net);
                        }

                        // length of args
                        int len_args_net;
                        int len_args;
                        recv(i, &len_args_net, 4, 0);
                        len_args = ntohl(len_args_net);
                        printf("len_args %d\n", len_args);

                        // stores args of different types
                        void **args;
                        args = (void **)malloc((len_argTypes/2 - 1) * sizeof(void *));
                        printf("size of args %d\n", len_argTypes/2 - 1);
                        printf("size of void* %lu\n", sizeof(void *));
                        printf("size of both %lu\n", (len_argTypes/2 - 1) * sizeof(void *));
                        // Last argType is always 0 so skip that one
                        for (j=0; j<len_argTypes/2 - 1; j++){
                            printf("loop %d arg type %d \n", j, argTypes[j]);
                            if (argTypes[j] == char_output || argTypes[j] == char_input){
                                char arg;
                                char arg_net[1];
                                recv(i, arg_net, 1, 0);
                                arg = arg_net[0];
                                printf("arg %c\n", arg);
                                args[j] = &arg;
                            } else if (argTypes[j] == short_output || argTypes[j] == short_input){
                                short arg;
                                short arg_net;
                                recv(i, &arg_net, 2, 0);
                                arg = ntohs(arg_net);
                                printf("arg %d\n", arg);
                                args[j] = &arg;
                            } else if (argTypes[j] == int_output || argTypes[j] == int_input){
                                printf("test int\n");
                                int arg;
                                int arg_net;
                                recv(i, &arg_net, 4, 0);
                                arg = ntohl(arg_net);
                                printf("arg %d\n", arg);
                                args[j] = (void *)&arg;
                            } else if (argTypes[j] == long_output || argTypes[j] == long_input){
                                long arg;
                                long arg_net;
                                recv(i, &arg_net, 4, 0);
                                arg = ntohl(arg_net);
                                printf("arg %ld\n", arg);
                                args[j] = &arg;
                            } else if (argTypes[j] == double_output || argTypes[j] == double_input){
                                double arg;
                                recv(i, &arg, 8, 0);
                                printf("arg %f\n", arg);
                                args[j] = &arg;
                            } else if (argTypes[j] == float_output || argTypes[j] == float_input){
                                float arg;
                                float arg_net;
                                recv(i, &arg_net, 4, 0);
                                arg = ntohl(arg_net);
                                printf("arg %f\n", arg);
                                args[j] = &arg;
                            } else {
                                printf("Error argType undefined %d\n", argTypes[j]);
                            }
                        }

                        // Now we have all the info we need to run the function
                        // So run it

                        char * functionName = name;
                        int res = -1;

                        if (strcmp(functionName, "f0") == 0) {
                            res = f0_Skel(argTypes, args);
                        } else if (strcmp(functionName, "f1") == 0) {
                            res = f1_Skel(argTypes, args);
                        } else if (strcmp(functionName, "f2") == 0) {
                            res = f2_Skel(argTypes, args);
                        } else if (strcmp(functionName, "f3") == 0) {
                            res = f3_Skel(argTypes, args);
                        } else if (strcmp(functionName, "f4") == 0){
                            res = f4_Skel(argTypes, args);
                        }
                        
                        free(name);
                    }
                    
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
    if ((rv = getaddrinfo(NULL, "0", &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
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
    printf("SERVER_ADDRESS %s\n", hostIP);    
    printf("SERVER_ADDRESS_PORT %d\n", htons(port_num));
    
    SERVER_ADDRESS = (char*)malloc(sizeof(hostIP));
    SERVER_ADDRESS = strncpy(SERVER_ADDRESS, hostIP, sizeof(hostIP));
    PORT = htons(port_num);

    freeaddrinfo(ai); // all done with this

    //create a thread for listening purposes
    //pthread_create(&clientThread, NULL, listenForClient, (void*)0);

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
    printf("START CONNECT TO BINDER\n");
    // Connect to binder
    int binder_port = atoi(getenv("BINDER_PORT"));
    struct hostent *binder_address = gethostbyname(getenv("BINDER_ADDRESS"));
    int sockfd = connectToSocket(binder_port, binder_address); 
    if (sockfd < 0){
        printf(":(\n");
        return -1;
    }

    // request_msg format: LOC_REQUEST len_name name len_argTypes argTypes  
    int i;

    send_integer(sockfd, LOC_REQUEST);
    send_string(sockfd, name);
    send_argTypes(sockfd, argTypes);
    printf("done sending to binder\n");

    int code = recv_integer(sockfd);
    if (code != LOC_SUCCESS){
        return -1;
    }

    int server_port = recv_integer(sockfd);
    char* server_addr = recv_string(sockfd);

    close(sockfd);
    printf("done receiving from binder\n");

    printf("START CONNECT TO SERVER\n");
    // Connect to server
    struct hostent *server_address = gethostbyname(server_addr);
    sockfd = connectToSocket(server_port, server_address);
    if (sockfd < 0){
        return -1;
    }

    // request_msg format: EXECUTE len_name name len_argTypes argTypes len_args args
    send_integer(sockfd, EXECUTE);
    send_string(sockfd, name);
    send_argTypes(sockfd, argTypes);
    send_args(sockfd, argTypes, args);
    printf("done sending to server\n");

    // Receive from server
    printf("done receiving from server");

    free(server_addr);
    return 0;
}

int rpcCacheCall(char* name, int* argTypes, void** args){
	return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f){
    // REGISTER server_identifier_len server_identifier port len_name name len_argTypes argTypes 
    printf("sending server info to binder\n");
    int len = len_argTypes(argTypes);
    printf("Size of argTypes %d", len);
    send_integer(sockfdBinder, REGISTER);
    send_string(sockfdBinder, SERVER_ADDRESS);
    send_integer(sockfdBinder, PORT);
    send_string(sockfdBinder, name);
    send_argTypes(sockfdBinder, argTypes);
    printf("done sending server info to binder\n");

    // recv either REGISTER_SUCCESS or REGISTER_FAILURE
    printf("receiving info from binder to server");
    int code = recv_integer(sockfdBinder);
    int error = recv_integer(sockfdBinder);
    printf("done receiving info from binder to server");

    if (code == REGISTER_FAILURE){
        return error;
    }
    return 0;
}

int rpcExecute(){
    listenForClient((void*)0);
    return 0;
}

int rpcTerminate(){
    free(SERVER_ADDRESS);
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

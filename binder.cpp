/*
	this file will have all the binder code
	add further comments
*/

#include "rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "0"   // port we're listening on, to be dynamically decided
// #define PORT "3600" // for some reason the dynamically allocated port was not accepting connections
static const size_t max_size= 256;

// struct database {

// 	vector serverFunctions;
// }

// struct serverFucntions {
// 	char* name;
// 	char* argTypes;
// }

//to follow code structure just follow the numbered comments

int getPort(struct sockaddr *sa);

int main(int argc, char* argv[]) {
	// 1. Binder starts and creates a pool to connect to as many servers and clients it needs to

	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    
    struct sockaddr_storage remoteaddr; // remote address
    socklen_t addrlen;

    char* buf;
    int nbytes;
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
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
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
    printf("SERVER_ADDRESS %s\n", hostIP);    
    printf("SERVER_PORT %d\n", htons(port_num));

    freeaddrinfo(ai); // all done with this


    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }



    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

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
                        
                        // 2. found a new connection, don't need to do anything here


                        // printf("selectserver: new connection from %s on "
                        //     "socket %d\n",
                        //     inet_ntop(remoteaddr.ss_family,
                        //         get_in_addr((struct sockaddr*)&remoteaddr),
                        //         remoteIP, INET6_ADDRSTRLEN),
                        //     newfd);
                    }
                } else {
                	// printf("handle data\n");
                    // 3. reading data sent by either server or client to decide what to do

                    // int len_msg;
                    // int len_msg_net;
                    
                    // if ((nbytes = recv(i, &len_msg_net, 4, 0)) == -1) {
                    //     perror("recv");
                    //     exit(1);
                    // }

                    // len_msg = ntohl(len_msg_net);

                    //     // printf("%d\n", len_msg_net);
                    //     // printf("%d\n", len_msg);

                    //     buf = malloc(sizeof(char)*len_msg);

                    // if ((nbytes = recv(i, buf, len_msg, 0)) <= 0) {

                    //     // got error or connection closed by client
                    //     if (nbytes == 0) {
                    //         // connection closed
                    //         // printf("selectserver: socket %d hung up\n", i);
                    //     } else {
                    //         perror("recv");
                    //     }
                    //     close(i); // bye!
                    //     FD_CLR(i, &master); // remove from master set
                    // } else {
                    //     // we got some data from a client
                        
                    //     printf("%s\n", buf);
                    //     char* str2 = changeToTitle(buf); 
                    //     // printf("to client: %s\n", str2);
                        
                    //     len_msg = strlen(str2) + 1;
                    //     // printf("%d\n", len_msg);
                    //     len_msg_net = htonl(len_msg);
                    //     // printf("%d\n", len_msg_net);
                    //     send(i, (char*)&len_msg_net, 4, 0);
			            
                    //     sendMessage(i, str2, len_msg);
                        
                    //     free(buf);
                    // }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

	return 0;
}

int getPort(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);   
}
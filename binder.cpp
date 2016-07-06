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
#include <vector>

#define PORT "0"   // port we're listening on, to be dynamically decided
// #define PORT "3600" // for some reason the dynamically allocated port was not accepting connections
static const size_t max_size= 256;

using namespace std;

struct serverFucntions {
	char* name;
	void* argTypes;
	int sockfd;
};

struct database {
	vector<serverFucntions> functions;
};

//to follow code structure just follow the numbered comments

void sendMessage(int s, char* buf, unsigned int len);

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
    char remoteIP[INET6_ADDRSTRLEN];
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
    printf("BINDER_ADDRESS %s\n", hostIP);    
    printf("BINDER_ADDRESS_PORT %d\n", htons(port_num));

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


                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                	// printf("handle data\n");
                    // 3. reading data sent by either server or client to decide what to do

                    int code;
                    int code_net;
                    
                    // Get code
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

                    if (code == LOC_REQUEST){
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
                            recv(i, &argType_net, 2, 0);
                            argTypes[i] = ntohl(argType_net);
                            printf("argType %d\n", argTypes[i]);
                        }

                        // Determine which server info to return here

                        // Stub code assuming we find a server
                        printf("loc_success %d\n", LOC_SUCCESS);
                        int loc_success_net = htonl(LOC_SUCCESS);
                        send(i, (char*)&loc_success_net, 4, 0);

                        int server_port = 1234;
                        int server_port_net = htonl(server_port);
                        printf("server port %d\n", server_port);
                        send(i, (char*)&server_port_net, 4, 0);

                        char* server_addr = "Some server address";
                        
                        int len_server_addr = strlen(server_addr);
                        int len_server_addr_net = htonl(len_server_addr);
                        printf("size of server_addr %d\n", len_server_addr);
                        send(i, (char*)&len_server_addr_net, sizeof(len_server_addr_net), 0);

                        send(i, server_addr, len_server_addr, 0);


                        free(name);


                    } else if (code == REGISTER){
                        // length of server_identifier
                        int len_server_identifier_net;
                        int len_server_identifier;
                        recv(i, &len_server_identifier_net, 4, 0);
                        len_server_identifier = ntohl(len_server_identifier_net);
                        printf("len_server_identifier%d\n", len_server_identifier);

                        // server_identifier
                        char * server_identifier = (char*)malloc(sizeof(char)*len_server_identifier);
                        recv(i, server_identifier, len_server_identifier, 0);
                        printf("server_identifier %s\n", server_identifier);

                        // server port
                        int server_port_net;
                        int server_port;
                        recv(i, &server_port_net, 4, 0));
                        server_port = ntohl(server_port_net);
                        printf("server_port %d", server_port);

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
                            recv(i, &argType_net, 2, 0);
                            argTypes[i] = ntohl(argType_net);
                            printf("argType %d\n", argTypes[i]);
                        }

                        // Register server info here

                        // Stub code assuming we register successfully
                        printf("register_success %d\n", REGISTER_SUCCESS);
                        int register_success_net = htonl(REGISTER_SUCCESS);
                        send(i, (char*)&register_success_net, 4, 0);

                        // This integer will hold any warnings or errors
                        int error = 0;
                        int error_net = htonl(error);
                        printf("error %d\n", error);
                        send(i, (char*)&error_net, 4, 0);
                        

                        free(name);

                    }

                        //char* str2 = changeToTitle(buf); 
                        // printf("to client: %s\n", str2);
                        
                        //len_msg = strlen(buf) + 1;
                        // printf("%d\n", len_msg);
                        //len_msg_net = htonl(len_msg);
                        // printf("%d\n", len_msg_net);
                        //send(i, (char*)&len_msg_net, 4, 0);
			            
                        //sendMessage(i, str2, len_msg);
                        
                        //free(buf);
                    //}
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

	return 0;
}

void sendMessage(int s, char* buf, unsigned int len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here
    if (n <0)
        perror("send");
}


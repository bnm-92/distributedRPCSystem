/*
	this file will have all the binder code
	add further comments
*/
#include "rpc.h"
#include "network.h"
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
#include <exception>
#include <algorithm>


#define PORT "0"   // port we're listening on, to be dynamically decided
// #define PORT "3600" // for some reason the dynamically allocated port was not accepting connections
static const size_t max_size= 256;

using namespace std;

int end1;
int sendTermMsg;
int breakWhile;
int numConnections;

struct serverFunction {
	char* name;
	int* argTypes;
	int sockfd;
    char* address;
    int localfd;
    int numArgs;
};

struct database {
	vector<serverFunction> functions;
};

vector<serverFunction>::size_type db_idx;

//to follow code structure just follow the numbered comments

void sendMessage(int s, char* buf, unsigned int len);

int findNumber(vector<int>servers, int s) {
    for (int i=0; i<servers.size(); i++) {
        if (servers[i] == s) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    end1 = 0;
    sendTermMsg = 0;
    numConnections = 0;
    breakWhile = 0;
	// 1. Binder starts and creates a pool to connect to as many servers and clients it needs to

    // Stores info about all of the servers
    database db;
    db_idx = -1;

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
        // fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    // print your server name and port here
    
    gethostname(hostIP, INET6_ADDRSTRLEN);
    printf("BINDER_ADDRESS %s\n", hostIP);    
    printf("BINDER_PORT %d\n", htons(port_num));

    freeaddrinfo(ai); // all done with this


    // listen
    if (listen(listener, 1000) == -1) {
        // perror("listen");
        exit(3);
    }



    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        if (1 == sendTermMsg) {
            sendTermMsg = 0;
            vector<int> servers;
            for (int i=0; i<db.functions.size(); i++) {
                int s = db.functions[i].localfd;
                if (findNumber(servers, s) == 1) {
                } else {
                    servers.push_back(s);
                }
            }
            // now we have a list of servers connected
            for(int i = 0; i < servers.size(); i++) {
                send_integer(servers[i], TERMINATE);
            }
        }
        
        if (breakWhile) {
            break;
        }

        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            // perror("select");
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
                        // perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        numConnections++;
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

                    int code;
                    int code_net;
                    
                    // Get code
                    if ((nbytes = recv(i, &code_net, 4, 0)) <= 0) {
                        //got error or connection closed by client
                        if (nbytes == 0) {
                            //connection closed - if it was a server remove from database
                            // printf("selectserver: socket %d hung up\n", i);
                            for(std::vector<serverFunction>::size_type j = 0; j != db.functions.size();) {
                                if(db.functions[j].localfd == i){
                                    db.functions.erase(db.functions.begin() + j);
                                } else {
                                    j++;
                                }
                            }
                        } else {
                            // perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        numConnections--;
                        if ((end1 == 1) && (numConnections == 0)) {
                            breakWhile = 1;
                        }
                        break;
                    }
                    code = ntohl(code_net);

                    if (code == LOC_REQUEST){
                        char* name = recv_string(i);
                        int* argTypes = recv_argTypes(i);
                        int len = len_argTypes(argTypes);

                        // Determine which server info to return
                        serverFunction s;
                        bool found_server = false;
                        vector<serverFunction>::size_type copy_idx = db_idx;
                        vector<serverFunction>::size_type size = db.functions.size();
                        db_idx++;
                        for(; db_idx%size != copy_idx && size != 0; db_idx++) {
                            if (strcmp(db.functions[db_idx%size].name,name) == 0 && len == db.functions[db_idx%size].numArgs){
                                bool same = true;
                                for (int j=0;j<len; j++){
                                    if (
                                        get_arg_type(argTypes[j]) != get_arg_type(db.functions[db_idx%size].argTypes[j]) ||
                                        is_input(argTypes[j]) != is_input(db.functions[db_idx%size].argTypes[j]) ||
                                        is_output(argTypes[j]) != is_output(db.functions[db_idx%size].argTypes[j]) ||
                                        (get_arg_length(argTypes[j]) > 0) !=  (get_arg_length(db.functions[db_idx%size].argTypes[j]) > 0)
                                    ){
                                        same = false;
                                    }
                                }
                                if (same){
                                    s = db.functions.at(db_idx%size);
                                    found_server = true;
                                    break;
                                }
                            }
                        }
                        db_idx = db_idx%size;
                        if (!found_server){
                            if (strcmp(db.functions[copy_idx].name,name) == 0 && len == db.functions[copy_idx].numArgs){
                                bool same = true;
                                for (int j=0;j<len; j++){
                                    if (
                                        get_arg_type(argTypes[j]) != get_arg_type(db.functions[copy_idx].argTypes[j]) ||
                                        is_input(argTypes[j]) != is_input(db.functions[copy_idx].argTypes[j]) ||
                                        is_output(argTypes[j]) != is_output(db.functions[copy_idx].argTypes[j]) ||
                                        (get_arg_length(argTypes[j]) > 0) !=  (get_arg_length(db.functions[copy_idx].argTypes[j]) > 0)
                                    ){
                                        same = false;
                                    }
                                }
                                if (same){
                                    s = db.functions.at(copy_idx);
                                    found_server = true;
                                    // printf("found a server with a matching function\n");
                                }
                            }
                        }
                        if (!found_server){
                            send_integer(i, LOC_FAILURE);
                        } else {
                            send_integer(i, LOC_SUCCESS);
                            send_integer(i, s.sockfd);
                            send_string(i, s.address);
                        }
                        free(name);

                    } else if (code == REGISTER){
                        try {
                            char* server_identifier = recv_string(i);
                            int server_port = recv_integer(i);
                            char* name = recv_string(i);

                            int *argTypes = recv_argTypes(i);
                            int len = len_argTypes(argTypes);

                            // Register server info to database
                            serverFunction server_function = {name, argTypes, server_port, server_identifier, i, len};
                            db.functions.push_back(server_function);
                            // Stub code assuming we register successfully
                            send_integer(i, REGISTER_SUCCESS);

                            // This integer will hold any warnings or errors
                            int error = 0;
                            send_integer(i, error);
                            
                            // Need to free these when server disconnects
                            //free(name);
                            //free(argTypes);
                        } catch (exception& e) {
                            send_integer(i, REGISTER_FAILURE);

                            // This integer will hold any warnings or errors
                            int error = -1;
                            send_integer(i, error);
                        }
                    } else if (code == TERMINATE) {
                        // loop through all servers and terminate
                        sendTermMsg = 1;
                        end1 = 1;
                    } else if (code == CACHE_REQUEST){
                        char* name = recv_string(i);
                        int* argTypes = recv_argTypes(i);
                        vector<serverFunction> matching_functions;
                        int len = len_argTypes(argTypes);
                        // find matching functions
                        for(std::vector<serverFunction>::size_type idx = 0; idx != db.functions.size(); idx++) {
                            if (strcmp(db.functions[idx].name,name) == 0 && len == db.functions[idx].numArgs){
                                bool same = true;
                                for (int j=0;j<len; j++){
                                    if (
                                        get_arg_type(argTypes[j]) != get_arg_type(db.functions[idx].argTypes[j]) ||
                                        is_input(argTypes[j]) != is_input(db.functions[idx].argTypes[j]) ||
                                        is_output(argTypes[j]) != is_output(db.functions[idx].argTypes[j]) ||
                                        (get_arg_length(argTypes[j]) > 0) !=  (get_arg_length(db.functions[idx].argTypes[j]) > 0)
                                    ){
                                        same = false;
                                    }
                                }
                                if (same){
                                    //printf ("%s %s\n", name, db.functions[idx].name);
                                    matching_functions.push_back(db.functions[idx]);
                                }
                            }
                        }
                        //Send matching functions
                        // printf("size %d\n", matching_functions.size());
                        if (matching_functions.size() > 0){
                            send_integer(i, CACHE_SUCCESS);
                            send_integer(i, matching_functions.size());
                            for (std::vector<serverFunction>::size_type idx = 0; idx != matching_functions.size(); idx++){
                                // printf("sending server...%s\n", matching_functions[idx].name);
                                send_server(i, matching_functions[idx]);
                            }
                        } else {
                            send_integer(i, CACHE_FAILURE);
                        }
                    }
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
    // if (n <0)
        // perror("send");
}


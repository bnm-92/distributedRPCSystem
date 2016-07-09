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

int numServerCon = 0;
int end2 = 0;
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

struct serverFunction {
    char* name;
    int* argTypes;
    int sockfd;
    char* address;
    int localfd;
    int numArgs;
};

vector<serverFunction> cache;

void printArgs(int* argTypes, void ** args) {
    for (int i=0; i<len_argTypes(argTypes)-1; i++) {
        printf("%d\n", *(int*)args[i]);
    }
}

int getargTypesLength(int* argTypes) {
	// write code here
	int n = 0;
	while (*argTypes != 0) {
		argTypes++;
		n++;
	}

	return n;
}

struct functionPair {
	char* name;
	int* argTypes;
};

struct functionMap {
	struct functionPair pair;
	skeleton skel;
};

std::vector<functionMap> functions;

bool checkFunction2(char* name, int* argTypes, functionPair pair) {
	if (strcmp(name, pair.name) == 0) {
		if (len_argTypes(argTypes) == len_argTypes(pair.argTypes)) {
			for (int i=0; i<len_argTypes(argTypes); i++) {
				// just check 
				if (
					(is_input(argTypes[i]) == is_input(pair.argTypes[i])) && 
                    (is_output(argTypes[i]) == is_output(pair.argTypes[i])) && 
					(get_arg_type(argTypes[i]) == get_arg_type(pair.argTypes[i])) && 
					((get_arg_length(argTypes[i]) > 0) == (get_arg_length(pair.argTypes[i]) > 0))
					) {
					return true;
				}
			}	
		}
	}
	return false;
}

skeleton checkFunction(char* name, int* argTypes) {
	skeleton skel = NULL;
	for (int i=0; i<functions.size(); i++) {
		if (checkFunction2(name, argTypes, functions[i].pair)) {
			skel = functions[i].skel;
			break;
		}
	}
	return skel;
}

void *listenForClient(void * id) {
    int i;
    // listen
    if (listen(listener, 10) == -1) {
        // perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    fdmax = listener;

    // main loop
    for(;;) {
        if (end2 && (numServerCon == 0)) {
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
                        numServerCon++;
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
                            // printf("selectserver: socket %d hung up\n", i);
                        } else {
                            // perror("recv");
                        }
                        numServerCon--;
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        break;
                    }
                    code = ntohl(code_net);
                    // printf("code %d\n", code);

                    if (code == EXECUTE){
                        char* name =  recv_string(i);
                        int* argTypes = recv_argTypes(i);

                        void** args = recv_args(i, argTypes);
                        // printf("\n printing final args\n");
                        // Now we have all the info we need to run the function
                        // So run it

                        skeleton skel = checkFunction(name, argTypes);
                        int res = -1;
                        if (skel) {
                        	res = skel(argTypes, args);	
                        }

                        //Success send back to client
                        if (res == 0) {
                            send_integer(i, EXECUTE_SUCCESS);
                            send_string(i, name);
                            send_argTypes(i, argTypes);
                            send_args(i, argTypes, args); 
                        } else {
                            send_integer(i, EXECUTE_FAILURE);
                        }
                        
                        //free(name);
                        //free(argTypes);
                        free(args);
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
    // printf("SERVER_ADDRESS %s\n", hostIP);    
    // printf("SERVER_ADDRESS_PORT %d\n", htons(port_num));
    
    SERVER_ADDRESS = (char*)malloc(sizeof(hostIP));
    strncpy(SERVER_ADDRESS, hostIP, sizeof(hostIP));
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
        // fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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
        // fprintf(stderr, "server: failed to connect\n");
        return 2;
    }

    inet_ntop(pB->ai_family, get_in_addr((struct sockaddr *)pB->ai_addr), sB, sizeof sB);
    // printf("server: connecting to %s\n", sB);
    // while(1) {
    //     //do some work
    // }
    freeaddrinfo(servinfoB);
    // printf("\nRPC INIT END\n");
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

    int binder_port = atoi(getenv("BINDER_PORT"));
    struct hostent *binder_address = gethostbyname(getenv("BINDER_ADDRESS"));

    int sockfd = connectToSocket(binder_port, binder_address); 
    if (sockfd < 0){
        return -1;
    }

    int i;
    send_integer(sockfd, LOC_REQUEST);
    send_string(sockfd, name);
    send_argTypes(sockfd, argTypes);

    int code = recv_integer(sockfd);
    if (code != LOC_SUCCESS){
        return -1; // LOC_FAILURE
    }

    int server_port = recv_integer(sockfd);
    char* server_addr = recv_string(sockfd);

    close(sockfd);
    
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


    // Receive from server after it executes
    code = recv_integer(sockfd);
    if (code == EXECUTE_SUCCESS){
        name = recv_string(sockfd);
        argTypes = recv_argTypes(sockfd);
        void ** args2 = (recv_args(sockfd, argTypes));

        for (int i=0; i<len_argTypes(argTypes)-1; i++) {

            args[i] = args2[i];
        }
    } else {
        return -1; //execution was not success
    }

    free(server_addr);
    return 0;
}

int rpcCallServer(struct serverFunction s, void** args){
    // Connect to server
    struct hostent *server_address = gethostbyname(s.address);
    int sockfd = connectToSocket(s.sockfd, server_address);
    if (sockfd < 0){
        return -1;
    }

    // request_msg format: EXECUTE len_name name len_argTypes argTypes len_args args
    send_integer(sockfd, EXECUTE);
    send_string(sockfd, s.name);
    send_argTypes(sockfd, s.argTypes);
    send_args(sockfd, s.argTypes, args);


    // Receive from server after it executes
    int code = recv_integer(sockfd);
    if (code == EXECUTE_SUCCESS){
        char* name = recv_string(sockfd);
        int* argTypes = recv_argTypes(sockfd);
        void ** args2 = (recv_args(sockfd, argTypes));

        for (int i=0; i<len_argTypes(argTypes)-1; i++) {
            args[i] = args2[i];
        }
    } else {
        return -1; //execution was not success
    }

    printf("\nRPC CALL END\n");
    return 0;
}

int rpcCallBinder(char* name, int* argTypes, void** args){
    printf("\nRPC CALL\n");

    // Remove all functions first
    for(vector<serverFunction>::size_type i = 0; i != cache.size();) {
        bool same = false;
        int len = len_argTypes(argTypes);
        if (strcmp(cache[i].name,name) == 0 && len == cache[i].numArgs){
            for (int j=0;j<len; j++){
                if (
                    get_arg_type(argTypes[j]) == get_arg_type(cache[i].argTypes[j]) &&
                    is_input(argTypes[j]) == is_input(cache[i].argTypes[j]) &&
                    is_output(argTypes[j]) == is_output(cache[i].argTypes[j]) &&
                    (get_arg_length(argTypes[j]) > 0) ==  (get_arg_length(cache[i].argTypes[j]) > 0)
                ){
                    same = true;
                }
            }
        }
        if (same){
            cache.erase(cache.begin() + i);
        } else {
            i++;
        }
    }

    int binder_port = atoi(getenv("BINDER_PORT"));
    struct hostent *binder_address = gethostbyname(getenv("BINDER_ADDRESS"));

    int sockfd = connectToSocket(binder_port, binder_address); 
    if (sockfd < 0){
        return -1;
    }

    send_integer(sockfd, CACHE_REQUEST);
    send_string(sockfd, name);
    send_argTypes(sockfd, argTypes);

    int code = recv_integer(sockfd);
    if (code != CACHE_SUCCESS){
        return -1;
    }

    int num_servers = recv_integer(sockfd);
    // printf("num servers %d\n", num_servers);
    struct serverFunction *servers = recv_servers(sockfd, num_servers);
    for (int i=0; i<num_servers; i++){
        cache.push_back(servers[i]);
        // printf("pushing %s\n\n", servers[i].name);
    }
    // printf("size cache %d\n", cache.size());

    close(sockfd);
}

int rpcCacheCall(char* name, int* argTypes, void** args){
    serverFunction s;
    int success = -1;
    int len = len_argTypes(argTypes);
    for(vector<serverFunction>::size_type i = 0; i != cache.size(); i++){
        if (strcmp(cache[i].name,name) == 0 && len == cache[i].numArgs){
            bool same = true;
            for (int j=0;j<len_argTypes(argTypes); j++){
                if (
                    get_arg_type(argTypes[j]) != get_arg_type(cache[i].argTypes[j]) ||
                    is_input(argTypes[j]) != is_input(cache[i].argTypes[j]) ||
                    is_output(argTypes[j]) != is_output(cache[i].argTypes[j]) ||
                    (get_arg_length(argTypes[j]) > 0) !=  (cache[i].argTypes[j] > 0)
                ){
                    same = false;
                }
            }
            if (same){
                s = cache[i];
                success = rpcCallServer(s, args);
                if (success == 0){
                    // printf("successfully hit cache and returned\n");
                    break;
                }
            }
        }
    }
    if (success != 0){
        // printf("didn't hit cache now pulling\n");
        rpcCallBinder(name, argTypes, args);
        // printf("size of cache %d\n", cache.size());
        for(vector<serverFunction>::size_type i = 0; i != cache.size(); i++){
            // printf("comparing %s %s %d %d\n", cache[i].name, name, len, cache[i].numArgs);
            if (strcmp(cache[i].name,name) == 0 && len == cache[i].numArgs){
                bool same = true;
                for (int j=0;j<len_argTypes(argTypes); j++){
                    int arg_len = 0;
                    if(get_arg_length(argTypes[j]) > 0){
                        arg_len = 1;
                    }
                    int cache_arg_len = 0;
                    if(get_arg_length(cache[i].argTypes[j]) > 0){
                        cache_arg_len = 1;
                    }
                    if (
                        (get_arg_type(argTypes[j]) != get_arg_type(cache[i].argTypes[j])) ||
                        (is_input(argTypes[j]) != is_input(cache[i].argTypes[j])) ||
                        (is_output(argTypes[j]) != is_output(cache[i].argTypes[j])) ||
                        (arg_len != cache_arg_len)
                    ){
                        same = false;
                    }
                }
                if (same){
                    s = cache[i];
                    success = rpcCallServer(s, args);
                    if (success == 0){
                        // printf("successfully hit cache and returned\n");
                        break;
                    }
                }
            }
        }
        if (success != 0){
            // printf("didn't hit cache again :(\n");
            return -1;
        }
    }
	return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f){
    // REGISTER server_identifier_len server_identifier port len_name name len_argTypes argTypes 
    int len = len_argTypes(argTypes);

    send_integer(sockfdBinder, REGISTER);
    send_string(sockfdBinder, SERVER_ADDRESS);
    send_integer(sockfdBinder, PORT);
    send_string(sockfdBinder, name);
    send_argTypes(sockfdBinder, argTypes);

    // recv either REGISTER_SUCCESS or REGISTER_FAILURE

    int code = recv_integer(sockfdBinder);
    int error = recv_integer(sockfdBinder);

    if (code == REGISTER_FAILURE){
        return error;
    } else {
    	//register was successful so lets add it to map

        struct functionPair pair;
    	pair.name = name;
    	pair.argTypes = argTypes;
    	struct functionMap map;
    	map.pair = pair;
    	map.skel = f;
    	functions.push_back(map);
    	
    }
    return 0;
}

int rpcExecute(){
    // printf("HELLO TEHRE ANOTHER THREAD\n");
    // listenForClient((void*)0);
    pthread_create(&clientThread, NULL, listenForClient, (void*)0);
    while(1) {
        int code = 0;

        code = recv_integer(sockfdBinder);

        if (code == TERMINATE) {

            end2 = 1;
            close(sockfdBinder);
            break;
        } else if(code < 0) {
            close(sockfdBinder);
            break;
        }
    }

    return 0;
}

int rpcTerminate(){

    int binder_port = atoi(getenv("BINDER_PORT"));
    struct hostent *binder_address = gethostbyname(getenv("BINDER_ADDRESS"));

    int sockfd = connectToSocket(binder_port, binder_address); 
    if (sockfd < 0){
        return -1;
    }
    // send terminate message to binder
    send_integer(sockfd, TERMINATE);

    close(sockfd);

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

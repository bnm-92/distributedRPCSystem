
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
 
// For Protocols
#define REGISTER        	1
#define REGISTER_SUCCESS	2
#define REGISTER_FAILURE	3
#define LOC_REQUEST     	4
#define LOC_SUCCESS     	5
#define LOC_FAILURE     	6
#define EXECUTE         	7
#define EXECUTE_SUCCESS 	8
#define EXECUTE_FAILURE 	9
#define TERMINATE       	10
#define CACHE_REQUEST		11
#define CACHE_SUCCESS		12
#define CACHE_FAILURE		13

void *get_in_addr(struct sockaddr *sa);
int getPort(struct sockaddr *sa);

void send_integer(int sockid, int val);

void send_string(int sockid, char* val);

void send_argTypes(int sockid, int* argTypes);

int recv_integer(int sockid);

char* recv_string(int sockid);

int* recv_argTypes(int sockid);

int len_argTypes(int* argTypes);

void send_args(int sockid, int* argTypes, void** args);

int get_arg_type(int argType);

int get_arg_length(int argType);

void** recv_args(int sockid, int* argTypes);

int is_input(int argType);

int is_output(int argType);

struct serverFunction* recv_servers(int sockid, int num_servers);

void send_server(int sockid, serverFunction server);

/*
 * rpc.h
 *
 * This file defines all of the rpc related infomation.
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
 
#define ARG_CHAR    1
#define ARG_SHORT   2
#define ARG_INT     3
#define ARG_LONG    4
#define ARG_DOUBLE  5
#define ARG_FLOAT   6

#define ARG_INPUT   31
#define ARG_OUTPUT  30

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

typedef int (*skeleton)(int *, void **);

extern int rpcInit();
extern int rpcCall(char* name, int* argTypes, void** args);
extern int rpcCacheCall(char* name, int* argTypes, void** args);
extern int rpcRegister(char* name, int* argTypes, skeleton f);
extern int rpcExecute();
extern int rpcTerminate();

void *get_in_addr(struct sockaddr *sa);
int getPort(struct sockaddr *sa);

#ifdef __cplusplus
}
#endif


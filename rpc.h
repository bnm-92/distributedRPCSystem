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
#define REGISTER        1
#define LOC_REQUEST     2
#define LOC_SUCCESS     3
#define LOC_FAILURE     4
#define EXECUTE         5
#define EXECUTE_SUCCESS 6
#define EXECUTE_FAILURE 7
#define TERMINATE       8

typedef int (*skeleton)(int *, void **);

extern int rpcInit();
extern int rpcCall(char* name, int* argTypes, void** args);
extern int rpcCacheCall(char* name, int* argTypes, void** args);
extern int rpcRegister(char* name, int* argTypes, skeleton f);
extern int rpcExecute();
extern int rpcTerminate();

void *get_in_addr(struct sockaddr *sa);

#ifdef __cplusplus
}
#endif


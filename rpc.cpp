#include <rpc.h>

typedef int (*skeleton)(int *, void **){
    return 0;
}

int rpcInit(){
    return 0;
}

int rpcCall(char* name, int* argTypes, void** args){
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


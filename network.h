void send_integer(int sockid, int val);

void send_string(int sockid, char* val);

void send_argTypes(int sockid, int* argTypes);

int recv_integer(int sockid);

char* recv_string(int sockid);

int* recv_argTypes(int sockid);

int len_argTypes(int* argTypes);

void send_args(int sockid, int* argTypes, void** args);

void** recv_args(int sockid, int* argTypes);
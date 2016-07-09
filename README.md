# cs654-3
group -> Babar Naveed Memon 20648640 bmemon@uwaterloo.ca
		 Ryan Catoen 20476358 rmcatoen@uwaterloo.ca

TO RUN THE PROJECT:
	FIRST USE MAKE
	THEN COMPILE SERVER AND CLIENT
	example:
	g++ -L. server_functions.o server_function_skels.o server.c -lrpc -o server -pthread
	g++ -L. client1.c -lrpc -o client1 -pthread

	the example in the assignment it doesnt include -pthread(please include that) and instead of server.o and client.o you need server.c and client.c because we aren't compiling server/client files in our makefile



Protocol:

Client/Binder protocol:

Client sends the following to Binder in rpcCall
	LOC_REQUEST
	function name
	argTypes

Binder receives the above info and checks for a server with a matching function
name and argTypes

Binder sends a status code and possibly server port and server address to Client

LOC_SUCCESS:
	The client receives the following info
		server port
		server address
LOC_FAILURE
	The client returns -1

Addionally the client may send TERMINATE to the binder, in this case
the binder will send a msg to all servers to terminate


Server/Binder protocol:

Server sends the following to Binder in rpcRegister
	REGISTER
	server address
	server port
	function name
	argTypes

Binder receives the above info and stores the following info about the server 
function in its database

	function name
	argTypes
	server socket
	server address
	socket that the server is connected to on the binder
	the number of arguments of the function

The binder sends a status code and error code back to the server

REGISTER_FAILURE:
	return error code

REGISTER_SUCCESS:
	the server adds a map for the skeleton function to be called later

Addionally the binder may send TERMINATE to the server, in this case the
server will terminate

Client/Server

Client sends the following info to a server that the binder determined was able
to run the requested function

	EXECUTE
	function name
	argTypes
	args

The server then looks up the function name in its map and calls the skeleton
function with the given information

The return value of the skeleton function is stored in res

If res is equal to 0 then we return EXCUTE_SUCCESS and the information below
Otherwise we return EXECUTE_FAILURE

EXECUTE_SUCCES:
	function name
	argTypes
	args

EXECUTE_FAILURE:
	/

The client receives the above information

EXECUTE SUCCESS:
	modifies the original args to contain the args sent back from the server

EXECUTE_FAILURE:
	return -1

Bonus - client caching

We implemented the bonus of caching the server location of functions on the client

Here is the flow of rpcCacheCall

Client checks if there is a matching function signature in its cache
If so we attempt to send an rpc call to the server
If it is successful we return 0

Otherwise we continue looping through all matching function signatures and repeating

If we are not successful by the time we reach the end of our cache the client sends the following to the binder after deleting all copies of the requested function signature in the cache

CACHE_REQUEST
function name
argTypes

The binder responds with either CACHE_SUCCESS if it finds at least one matching function signature or CACHE_FAILURE if it does not

CACHE_SUCCESS:
	Client receives the following info:
		number of matching functions
		That number of serialized serverFunction structs
	The client then loads the serverFunction structs into the cache

	Client checks if there is a matching function signature in its cache
	If so we attempt to send an rpc call to the server
	If it is successful we return 0

	Otherwise we continue looping through all matching function signatures and repeating

	If we are not successful by the time we reach the end of our cache then we return 0


CACHE_FAILURE:
	return -1



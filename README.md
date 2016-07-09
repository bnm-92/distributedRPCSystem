# cs654-3
group -> Babar Naveed Memon 20648640 bmemon@uwaterloo.ca
		 Ryan Catoen 20476358 rmcatoen@uwaterloo.ca

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


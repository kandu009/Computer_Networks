
all:
	
	g++ ./credentialserver/credential_main.cxx  ./credentialserver/src/credential_server.cxx ./server/src/server.cxx ./server/src/socket.cxx -lpthread -o bcredential

	g++ ./server/server_main.cxx ./server/src/server.cxx ./server/src/socket.cxx ./client/src/client.cxx -lpthread -o bcontent

	g++ ./client/client_main.cxx ./client/src/client.cxx ./server/src/server.cxx ./server/src/socket.cxx -o bclient

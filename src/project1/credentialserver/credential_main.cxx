#include "./include/credential_server.h"
#include "../utilities/common.h"
#include <iostream>
#include <string>

using namespace std;
using namespace responsecodes;

/**
 * class to hold the socket and the server needed by every request sent to the server
 *
 * @ m_socket 		: socket on which the client communication takes place
 * @ m_credServer	: Credential server object
 *
 */
class ThreadArgs {
	public:
		ThreadArgs(Socket& s, CredentialServer& cs) {
			m_socket = s;
			m_credServer = cs;
		};
		Socket& getSocket() {
			return m_socket;
		}
		CredentialServer& getCredentialServer() {
			return m_credServer;
		}
	private:
		Socket m_socket;
		CredentialServer m_credServer;
};

/**
 * Handler which handles new requests receieved by the server
 * @ta : Thread Args object
 */
void* connection_handler(void *ta) {

	ThreadArgs *args = (ThreadArgs*)ta;

	CredentialServer& cs = args->getCredentialServer();
	Socket &client_socket = args->getSocket();		

	while(1) {

		std::string request, response;
		client_socket.recv(request);

		if(0 == request.find("exit")) {			// exit request : send an exit request to credential server and respond back to client
			cs.exit(request, response);
			client_socket.send(response);
			break;
		} else if(0 == request.find("register") || 0 == request.find("login")) {	//login/register : validate and send it to credential server
			if(cs.validateClient(request, response)) {
				cs.loginOrRegister(request, response);	
			}
			client_socket.send(response);
		} else if(0 == request.find("list")) {	// delegate the list command to credential server
			std::string output;
			cs.list(output, response);
			client_socket.send(response.append("|").append(output));
		} else {
			response = ERR_400;					// invalid request received from client
			client_socket.send(response);
		}
	}
}

int main(int argc, char *argv[]) {

	CredentialServer cs;					// credential server which is the controller of this process
	cs.initialize();			

	while(1) {
		Socket client_socket;
		if(!cs.accept(client_socket)) {		// accept new connections from client
			cout << "credentialserver:: error:: could not accept the client connection." << endl;
			continue;
		}

		pthread_t thread_id;
		ThreadArgs ta(client_socket, cs);	// spawn one thread  for each request coming in
		int result = pthread_create(&thread_id, NULL, connection_handler, reinterpret_cast<void*>(&ta));
		if(result < 0) {
			cout << "credentialserver:: error:: could not create client thread" << endl;
			continue;
		}
	}

	return 0;

}

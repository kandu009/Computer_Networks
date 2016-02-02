#include "./include/server.h"
#include "./include/helper.h"
#include "../utilities/common.h"
#include "../client/include/client.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace responsecodes;

/**
  * Class to hold all the TCP request related information which needs to be
  * passed on to a thread when server receives a request
  */
class TcpThreadArgs {
	public:
		// constructor
		TcpThreadArgs(std::string& request, Socket& s, Server& cs) : 
			m_request(request),
			m_socket(s),
			m_server(cs) {
			};

		// getter methods
		Socket& getClientSocket() {
			return m_socket;
		}
		Server& getServer() {
			return m_server;
		}
		std::string getRequest() {
			return m_request;
		}
	private:
		Socket m_socket;		// client socket
		Server m_server;		// server object
		std::string m_request;	// request sent by the client
};

/**
  * Class to hold all the UDP request related information which needs to be
  * passed on to a thread when server receives a request
  */
class UdpThreadArgs {
	public:
		// constructor
		UdpThreadArgs(std::string& request, Socket& s, Server& ts, Server& us):
			m_request(request),
			m_socket(s),
			m_tcpServer(ts),
			m_udpServer(us) {
			};

		// getter methods
		Socket& getClientSocket() {
			return m_socket;
		}
		Server& getTcpServer() {
			return m_tcpServer;
		}
		Server& getUdpServer() {
			return m_udpServer;
		}
		std::string getRequest() {
			return m_request;
		}

	private:
		Socket m_socket;			// client socket
		Server m_tcpServer;			// TCP server on which the original request has been received
		Server m_udpServer;			// UDP server on which the further communication takes place
		std::string m_request;		// request sent by the client
};

/**
  * Handler which deals with the new TCP requests receieved by the server
  *  @ta : Thread Args object
  */
void* tcp_connection_handler(void *ta) {

	TcpThreadArgs *args = (TcpThreadArgs*)ta;

	std::string request = args->getRequest();
	Server& cs = args->getServer();
	Socket &client_socket = args->getClientSocket();		

	while(1) {

		std::string response;

		if(0 == request.find("exit")) {		// pass on the exit request to credential server and communicate response to client
		
			cs.exit(request, response);
			client_socket.send(response);

		} else if(0 == request.find("get")) {	// server the file get request

			std::vector<std::string> toks = Common::tokenize(request, const_cast<const char *>(" "));
			if(4 != toks.size()) {				// inform client in case of invalid requests
				response.append(ERR_405);
				client_socket.send(response);
			}
			// send the file requested
			cs.sendFile(client_socket, toks[1], toks[2], toks[3], response); 
			
			// send the response to the client
			client_socket.send(response);

		} else if(0 == request.find("probe")) {

			response.append("ACK : ").append(request);
			client_socket.send(response);

		} else {

			response = ERR_400;
			client_socket.send(response);

		}
		// receive the new requests until client exits.
		client_socket.recv(request);
	}
}

/**
  * Handler which deals with the new UDP requests receieved by the server
  *  @ta : Thread Args object
  */
void *udp_connection_handler(void *ta) {

	UdpThreadArgs *args = (UdpThreadArgs*)ta;

	std::string request = args->getRequest();
	Server& ts = args->getTcpServer();
	Server &us = args->getUdpServer();
	Socket &client_socket = args->getClientSocket();		

	while(1) {

		std::string response;

		if(0 == request.find("get")) {		// serve the get request and respond back to client

			std::vector<std::string> toks = Common::tokenize(request, const_cast<const char *>(" "));
			if(6 != toks.size()) {			// inform client incase of invalid requests
				response.append(ERR_405);
				client_socket.send(response);
			}

			Client udpclient(true);			// send the file and respond back to client with the status
			udpclient.connectToServer(toks[4], atoi(toks[5].c_str()));
			us.sendFile(udpclient.getObjSocket(), toks[1], toks[2], toks[3], response);
			client_socket.send(response);

		} else if(0 == request.find("probe")) { 
			
			// validate the probe message and inform back to client in case of any discrepancies
			std::vector<std::string> toks = Common::tokenize(request, const_cast<const char *>(" "));
			if(6 != toks.size()) {
				response.append(ERR_405);
				client_socket.send(response);
			}

			// In case of probe messages, we just need to send back the ACK's
			Client udpclient(true);
			udpclient.connectToServer(toks[4], atoi(toks[5].c_str()));
			response.append("ACK : ").append(request);
			udpclient.sendData(response);

		} else {
			// this means the request is not handled ot invalid
			response = ERR_400;
			client_socket.send(response);
		}

		// receive the next request until client exits
		client_socket.recv(request);
	}
}

/*
 * checks if the request is a udp or tcp request. We check for tcp/udp token
 * in the 4th argument as the commands supported by server are expected
 * to be aligned in the below mentioned format for both the cases.
 *
 * case 1 : in case of get command <get> <file> <server> <tcp/udp>
 * case 2 : in case of probe messages <probe> <message> <time> <tcp/udp>
 */
bool isUdpRequest(std::string& req) {

	std::vector<std::string> toks = Common::tokenize(req, const_cast<const char *>(" "));
	if(toks.size() < 4) {
		return false;
	}
	if(toks[3] == "udp") {
		return true;
	} else {
		return false;
	}
}

/**
  * method to load all the configuration information at the time of starting the server
  * @tcpport : port on which TCP server runs
  * @udpport : port on which UDP server runs
  */
void loadServerConfiguration(int& tcpport, int& udpport) {

	// open the config file
	string file = "content_server.cfg";
	ifstream myfile;
	myfile.open(Common::getFullPath(file).c_str());
	string line;

	if(!myfile.is_open()) {
		cout << "content server log:: warn:: could not load the config file" << endl;
		return;
	}

	while(getline(myfile, line)) {
		if(line.find("tcpport=") == 0) { // extract tcp port information
			std::vector<std::string> toks = Common::tokenize(line, const_cast<const char *>("="));
			tcpport = atoi(toks[1].c_str());

		} else if(line.find("udpport=") == 0) { // extract udp port information
			std::vector<std::string> toks = Common::tokenize(line, const_cast<const char *>("="));
			udpport = atoi(toks[1].c_str());
		}
	}

	//close the config file
	myfile.close();
}


int main(int argc, char *argv[]) {

	int tcpport = 55555;
	int udpport = 55556;

	// load the configs
	loadServerConfiguration(tcpport, udpport);

	// start the tcp server
	Server ts(tcpport);
	ts.initialize();
	ts.listen();

	// start the udp server
	Server us(udpport, true);
	us.initialize();

	while(1) {

		// accepts all requests in tcp
		Socket client_socket;
		if(!ts.accept(client_socket)) {
			cout <<"Could not accept client connection" << endl;
			continue;
		}

		std::string request;
		client_socket.recv(request);
		int result = -1;
		pthread_t thread_id;

		if(isUdpRequest(request)) {		// in case of udp request, delegate the task to the udp server
			// create a thread to server the udp request
			UdpThreadArgs *ta = new UdpThreadArgs(request, client_socket, ts, us);
			result = pthread_create(&thread_id, NULL, udp_connection_handler, reinterpret_cast<void*>(ta));
		} else {
			// create a thread to server the tcp request
			TcpThreadArgs *ta = new TcpThreadArgs(request, client_socket, ts);
			result = pthread_create(&thread_id, NULL, tcp_connection_handler, reinterpret_cast<void*>(ta));
		}
		if(result < 0) { // continue in case of a failure while creating thread
			cout << "Error in creating thread" << endl;
			continue;
		}
	}

	return 0;

}

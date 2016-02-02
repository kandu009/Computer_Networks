#ifndef __H_CLIENT_H__
#define __H_CLIENT_H__

#include <string>  //string
#include <arpa/inet.h> //inet_addr
#include "../../utilities/common.h"
#include "../../server/include/socket.h"
#include <map>
#include <set>

using namespace responsecodes;

const int MAXRECV_SIZE = 500;

class Client {
	private:
		int m_socket;													// socket of the client to communicate to server
		std::string m_address;											// server host address
		int m_port;														// port
		bool m_isUDP;													// flag to identify if its a tcp or udp client
		struct sockaddr_in m_server;									// socket address
		std::map<std::string, std::pair<int, int> > m_serverVsTcpUdpTime;// server vs its tcp and udp RTT's	
		std::set<std::string> m_contentServers;							// set of content servers

	public:
		Client(bool is_udp=false);										// constructor
		bool connectToServer(std::string address, int port);			// connect to server 
		bool sendData(std::string data);								// send data over connection
		int receive(std::string &response);								// receive data over connection
		bool receiveFile(std::string &file, std::string &response);		// receive file from server
		int getSocket() const { return m_socket; };						// get the socket fd
		
		Socket& getObjSocket() {										// get the socket object
			Socket *s = new Socket(m_isUDP, m_server, m_socket);
			return *s;
		};

	private:
		int getIpByHost(std::string &host, std::string &ip); 				// gets IP address from hostname

};

#endif /*__H_CLIENT_H__*/

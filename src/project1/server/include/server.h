#ifndef __H_SERVER_H__
#define __H_SERVER_H__

#include "socket.h"
#include <set>

/**
  * Class to extract all the server related logic and objects
  */
class Server {

	public:
		// Constructors	
		Server(int port, bool is_udp=false);
		Server(std::string& host, int port, bool is_udp=false);

		// initialization methods
		void loadConfig();
		bool initialize();
		bool listen();
		bool accept(Socket &client_socket);

		// data transfer methods
		bool send(std::string& data);
		bool receive(std::string& data);
		bool sendFile(Socket &client_socket, std::string &filename, 
				std::string &server, std::string &protocol, std::string &response);
		bool receiveFile(std::string &file, std::string &response);

		// action methods
		void exit(std::string& input, std::string& response);
	
		// getters and setters
		void setConnectionType(bool is_udp) {
			m_isUDP = is_udp;
			m_socket.setConnectionType(is_udp);	
		};
		std::string getHostname() { return m_host; };

		Socket getSocket() { return m_socket; };

	private:
		std::string m_host;						// host name
		int m_port;								// port number
		bool m_isUDP;							// flag to indicate if its a TCP or UDP server
		Socket m_socket;						// socket 
		std::set<std::string> m_hostedFiles;	// list of files hosted by server
};

#endif /*__H_SERVER_H__*/

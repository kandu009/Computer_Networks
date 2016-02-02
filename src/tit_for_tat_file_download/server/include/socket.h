// Definition of the Socket class

#ifndef __H_SOCKET__
#define __H_SOCKET__


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

const int MAXCONNECTIONS = 10;
const int MAXRECV = 500;

class Socket
{
	public:
		// constructors
		Socket(bool isUDP = false);
		Socket(const Socket &s);
		Socket(bool isUDP, struct sockaddr_in server, int sockfd);

		// operators
		Socket& operator=(const Socket &s);

		// Server initialization
		bool create();
		bool bind(std::string& host, const int port);
		bool listen() const;
		bool accept(Socket&) const;
		bool close();

		// Data Transimission
		bool send(const std::string) const;
		int recv(std::string&) const;
		
		bool is_valid() const { return m_sock != -1; };

		// getters and setters
		void setConnectionType(bool is_udp) { m_isUDP = is_udp; };
		int getSocketAddress() const { return m_sock; };

	private:
		// gets the IP address from the hostname
		int getIpByHost(std::string &host, std::string &ip);

	private:
		int m_sock;					// socket fd
		sockaddr_in m_serveraddr;	// socket address
		bool m_isUDP;				// flag that indicates if its for tcp or udp requests


};


#endif /*__H_SOCKET__*/


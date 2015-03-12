// Implementation of the Socket class.

#include <iostream>
#include "../include/socket.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>

// Constructor
Socket::Socket(bool isUDP) : m_sock(-1), m_isUDP(isUDP) {
	memset (&m_serveraddr, 0, sizeof(m_serveraddr));
}

// Constructor
Socket::Socket(bool isUDP, struct sockaddr_in server, int sockfd):
	m_sock(sockfd), m_serveraddr(server), m_isUDP(isUDP) {
}	

// Copy constructor
Socket::Socket(const Socket &s) {
	m_sock = s.m_sock;
	m_serveraddr = s.m_serveraddr;
	m_isUDP = s.m_isUDP;
}

// = operator
Socket& Socket::operator=(const Socket &s) {
	m_sock = s.m_sock;
	m_serveraddr = s.m_serveraddr;
	m_isUDP = s.m_isUDP;
	return *this;
}

// method to get the IP from the host information
int Socket::getIpByHost(std::string &host, std::string &ip) {
	if(inet_addr(host.c_str()) == -1) {			// check if the host is already resolved
		hostent *record = gethostbyname(host.c_str());
		if(record == NULL) {					// return in case of invalid IP's
			std::cout << "Could not get the host name. " << std::endl;
			return -1;
		} 
		in_addr * address = (in_addr * )record->h_addr;		// translate the hostname to IP
		ip = inet_ntoa(* address);
	} else { // directly use in case if the IP is already resolved
		ip = host;
	}
	return 0;
}

// method to create a socket
bool Socket::create()
{
	// create a socket
	m_sock = m_isUDP ? socket(AF_INET, SOCK_DGRAM, 0) : socket(AF_INET, SOCK_STREAM, 0);
	if(!is_valid()) {
		return false;
	}

	// set socket options
	int on = 1;
	if(setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,(const char*)&on, sizeof(on)) == -1) {
		return false;
	}

	return true;
}

// method to bind the socket to a given host and port
bool Socket::bind(std::string& host, const int port) {

	if(!is_valid()) {
		return false;
	}

	// translate the hostname to IP
	m_serveraddr.sin_family = AF_INET;
	std::string ip;
	if(-1 == getIpByHost(host, ip)) {
		return false;
	}
	m_serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
	m_serveraddr.sin_port = htons(port);

	// bind the socket
	int bind_return = ::bind(m_sock, (struct sockaddr*)&m_serveraddr, sizeof(m_serveraddr));
	if(bind_return == -1) {
		return false;
	}

	return true;
}

// method to listen on the socket
bool Socket::listen() const {

	if(!is_valid()) {
		return false;
	}

	int listen_return = ::listen(m_sock, MAXCONNECTIONS);
	if(listen_return == -1) {
		return false;
	}

	return true;
}

// method to accept the incoming requests from the clients
bool Socket::accept(Socket& new_socket) const {

	int addr_length = sizeof ( m_serveraddr );
	new_socket.m_sock = ::accept(m_sock, (sockaddr *)&m_serveraddr, (socklen_t *)&addr_length);

	if (new_socket.m_sock <= 0) {
		return false;
	}
	return true;
}

// method to send data over the socket
bool Socket::send(const std::string ss) const {
	std::string s;
	s.append(ss).append("\r\n");

	// use sendto in case of UDP, send otherwise
	int status = m_isUDP ?
		(::sendto(m_sock, s.c_str(), s.size(), 0, (struct sockaddr*)&m_serveraddr, sizeof(m_serveraddr))) :
		(::send(m_sock, s.c_str(), s.size(), MSG_NOSIGNAL));
	if(status == -1) {
		// return in case of failure
		return false;
	}
	return true;
}

// method to receive data on the socket
int Socket::recv(std::string& s) const {

	char buf [ MAXRECV + 1 ];
	s = "";
	memset(buf, 0, MAXRECV + 1);
	socklen_t len = sizeof(m_serveraddr);

	// use recfrom in case of udp, recv otherwise
	int status = m_isUDP ?
		(::recvfrom(m_sock, buf, MAXRECV, 0, (struct sockaddr*)&m_serveraddr, &len)) :
		(::recv(m_sock, buf, MAXRECV, 0));

	if(-1 == status) {
		// return in case of failure
		return 0;
	}

	if(0 == status) {
		return 0;
	} 

	s = buf;
	s = s.substr(0, s.rfind("\r\n"));
	return status;
}

// method to close the socket
bool Socket::close() {
	return (-1 == ::close(m_sock)) ? false : true;
}

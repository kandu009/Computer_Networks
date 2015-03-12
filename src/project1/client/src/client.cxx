#include "../include/client.h"
#include <cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sstream>
#include <sys/time.h>

using namespace std;

// Constructor
Client::Client(bool is_udp) : m_socket(-1), m_address(" "), m_port(0), m_isUDP(is_udp) {
	memset(&m_server, '0', sizeof(m_server)); 
}

// Gets the IP from hostname
// @host : input hostname
// @ip   : translated IP address
int Client::getIpByHost(std::string &host, std::string &ip) {
	hostent * record = gethostbyname(host.c_str());
	if(record == NULL) {
		std::cout << "Could not get the host name. " << std::endl;
		return -1;
	}
	in_addr * address = (in_addr * )record->h_addr;
	ip = inet_ntoa(* address);
	return 0;
}

// establishes a connection to the mentioned server
// @address : hostname of the server
// @port 	: port of the server
bool Client::connectToServer(std::string address, int port) {

	if(address.empty() || port < 0) {
		cout << "Invalid host/port information. Please check again ..." << endl; 
		return false;
	}

	m_address = address;
	m_port = port;

	//create socket if it is not already created
	if(m_socket == -1) {
		//Create socket
		m_socket = m_isUDP ? socket(AF_INET, SOCK_DGRAM, 0) : socket(AF_INET, SOCK_STREAM, 0);
		if (m_socket == -1)	{ 
			cout << "Could not create socket" << endl;
			return false;
		}       
	}

	m_server.sin_family = AF_INET;
	m_server.sin_port = htons(m_port);

	// resolve the hostname
	if(inet_addr(m_address.c_str()) == -1) {
		std::string ip_address;
		if(-1 == getIpByHost(m_address, ip_address)) {
			cout << "Failed to resolve hostname" << std::endl;
			return false;
		}
		m_server.sin_addr.s_addr = inet_addr(ip_address.c_str());
	} else {
		m_server.sin_addr.s_addr = inet_addr(m_address.c_str());
	}

	// connect only if its a tcp connection
	if(!m_isUDP && connect(m_socket, (struct sockaddr *)&m_server, sizeof(m_server)) < 0) {
		cout << "connect() method failed" << std::endl;
		return false;
	}

	return true;
}

// method to send data to the server
bool Client::sendData(std::string d) {

	std::string data;
	data.append(d).append("\r\n");

	// use send if its a tcp client. else use sendto
	int status = m_isUDP ? 
		(sendto(m_socket, data.c_str(), data.size(), 0, (struct sockaddr*)&m_server, sizeof(m_server))) :
		(send(m_socket, data.c_str(), data.size(), 0));
	if(-1 == status) {
		// return in case of failure
		cout << "Failed to send message [ " << data << " ]" << std::endl;
		return false;
	}
	return true;
}

// method to receive the data from a server
int Client::receive(std::string& response) {

	char buffer[MAXRECV_SIZE];
	socklen_t len = sizeof(m_server);
	// use recvfrom if its a udp client. recv otherwise
	int status = m_isUDP ?
		(::recvfrom(m_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&m_server, &len)) :
		(::recv(m_socket, buffer, sizeof(buffer), 0));

	response = "";
	if(-1 == status) {
		// return in case of failure
		cout << "recv failed" << endl;;
		return 0;
	}
	if(0 == status) {
		return 0;
	}

	// populate the output response
	response = buffer;
	response = response.substr(0, response.rfind("\r\n"));

	return status;
}

// method to receive the file from the server
bool Client::receiveFile(std::string &file, std::string &response) {

	FILE *fp;
	fp = fopen(file.c_str(), "ab");
	if(file.empty() || NULL == fp) {
		response = ERR_411;
		return false;
	}

	// divide the data into chunks and transfer in packets
	bool anyDataReceived = false;
	std::string data;
	while(receive(data) > 0) {
		anyDataReceived = true;
		cout << "Bytes received : " << data.size() << endl;
		fwrite(data.c_str(), 1, data.size(), fp);
		if(data.find("\r\n\r") != string::npos) {
			response = ERR_200;
			break;
		}
	}

	fclose(fp);
	if(!anyDataReceived) {
		// if no data is received, then there is something wrong
		cout << " No data received in client"<< endl;
		response = ERR_410;
		return false;
	}

	// everything went well
	response = ERR_200;
	return true;
}

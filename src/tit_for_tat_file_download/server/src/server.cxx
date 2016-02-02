#include "../include/server.h"
#include "../../utilities/common.h"
#include "iostream"
#include <fstream>
#include <stdio.h>

using namespace std;
using namespace responsecodes;

/**
 * MEethod to load the config information
 */
void Server::loadConfig() {

	// open the config file
	ifstream myfile;
	std::string file = "content_server.cfg";
	myfile.open(Common::getFullPath(file).c_str());
	string line;
	if(!myfile.is_open()) {
		cout << "server log:: warn:: could not load the config file" << endl;
		return;
	}

	while(getline(myfile, line)) {

		if(line.find("files=") == 0) {
			// extract the server, list of files
			std::vector<std::string> toks = Common::tokenize(line, const_cast<const char *>("="));
			if(2 != toks.size()) {
				continue;
			}
			// get the set of files for a given server
			std::vector<std::string> fs = Common::tokenize(toks[1], const_cast<const char *>(","));
			for(int i=0; i< fs.size(); ++i) {
				m_hostedFiles.insert(fs[i]);
			}
		}
	}
	// close the file
	myfile.close();
}

// constructor
Server::Server(std::string& host, int port, bool is_udp): m_host(host), m_port(port), m_isUDP(is_udp) {
	setConnectionType(is_udp);
	loadConfig();
}

// constructor
Server::Server(int port, bool is_udp) : m_port(port), m_isUDP(is_udp) {
	char buf[32];
	gethostname(buf,sizeof buf);
	m_host = std::string(buf);
	setConnectionType(is_udp);
	loadConfig();
}

// init method for the server
bool Server::initialize() {
	if(!m_socket.create()) {
		cout << "Could not create a socket" << endl;
		return false;
	}
	if(!m_socket.bind(m_host, m_port)) {
		cout << "Could not bind to port [ " << m_port << " ]" << endl;
		return false;
	}
	return true;
}

// listen method for socket
bool Server::listen() {
	if(!m_socket.listen()) {
		cout << "Could not listen on socket." << endl;
		return false;
	}
	return true;
}

// method to accept the conditions on the socket
bool Server::accept(Socket &client_socket) {

	if(!m_socket.accept(client_socket)) {
		cout << "Could not accept client request ... " << endl;
		return false;
	}
	return true;

}

// send data over the socket
bool Server::send(std::string& data) {
	return m_socket.send(data);
}

// receive data over the socket
bool Server::receive(std::string& data) {
	return m_socket.recv(data);
}

// method to handle the exit request
void Server::exit(std::string& input, std::string& response) {

	// validate the input request
	std::vector<std::string> toks = Common::tokenize(input, const_cast<const char *>(" "));
	if(2 != toks.size()) {
		response.append(ERR_400).append("Exit command is incorrect!");
		return;
	}

	// close the socket id
	int sockId = atoi(toks[1].c_str());
	int r = close(sockId);
	if(-1 == r) {
		cout << "Failed to close the socket. " << endl;
		response.append(ERR_414);
		return;
	}

	response.append(ERR_200);
}

// method to handle the file get command
bool Server::sendFile(Socket& client_socket, std::string &filename, 
		std::string &server, std::string &protocol, std::string &response) {

	// return if the file name is empty
	if(filename.empty()) {
		std::cout << "File transfer failed" << std::endl;
		response = ERR_410;
		return false;
	}

	// open the file that needs to be transferred
	FILE *fp = fopen(Common::getFullPath(filename).c_str(), "rb");
	if((m_hostedFiles.find(filename) == m_hostedFiles.end()) || fp == NULL) {
		std::cout << "File is not found to download [ " << filename << " ]" << endl;
		response = ERR_404;
		return false;
	}

	while(1) {

		// transfer the file in chunks of 256	
		unsigned char buff[256] = {0};
		int nread = fread(buff, 1, 256, fp);
		if(nread > 0) {
			cout << "Sending ... " << endl;
			std::string s;
			s.assign(reinterpret_cast<char*>(buff), nread);
			client_socket.send(s);
		}

		// Either there was error, or we reached end of file.
		if(nread < 256) {
			if(feof(fp)) {
				cout << "Done Sending ... " << endl;
				client_socket.send("\r\n\r");
				return true;
			}
			if(ferror(fp)) {
				response = ERR_410;
				// append reason for file download failure
				response.append(" Failed to retrieve the data from file.");
				client_socket.send(response);
				return false;
			}
		}
	}
}

// method to receiev files
bool Server::receiveFile(std::string &file, std::string &response) {

	// return if the filename is empty
	FILE *fp;
	fp = fopen(file.c_str(), "ab");
	if(file.empty() || NULL == fp) {
		response = ERR_411;
		response.append(" Filename is Empty.");
		return false;
	}

	// receive data in chunks and write to the file
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

	// close the file
	fclose(fp);
	if(!anyDataReceived) {
		// report if no data has been received
		response = ERR_410;
		response.append(" File is Empty.");
		return false;
	}

	// everything went well
	response = ERR_200;
	return true;
}
